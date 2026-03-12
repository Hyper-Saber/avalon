module;
#include <debug/assert.hpp>
#include <expected>
#include <functional>
#include <optional>
#include <vulkan/vulkan.h>

module avalon.rhi.vulkan;

import avalon.core;
import avalon.rhi;
import :utils;

import :command_buffer;

namespace avalon::rhi {

VkRhi::VkRhi() = default;

VkRhi::~VkRhi() {
  vkDeviceWaitIdle(m_deviceContext->GetDevice());
  for (auto &syncObject : m_frameSyncObjects) {
    if (syncObject.renderFinishedSemaphore != VK_NULL_HANDLE)
      vkDestroySemaphore(m_deviceContext->GetDevice(),
                         syncObject.renderFinishedSemaphore, nullptr);
    if (syncObject.imageAvailableSemaphore != VK_NULL_HANDLE)
      vkDestroySemaphore(m_deviceContext->GetDevice(),
                         syncObject.imageAvailableSemaphore, nullptr);
    if (syncObject.m_inflightFence != VK_NULL_HANDLE)
      vkDestroyFence(m_deviceContext->GetDevice(), syncObject.m_inflightFence,
                     nullptr);
  }

  for (auto &pool : m_frameCommandPools) {
    if (pool != VK_NULL_HANDLE)
      vkDestroyCommandPool(m_deviceContext->GetDevice(), pool, nullptr);
  }

  if (m_immTransferPool != VK_NULL_HANDLE)
    vkDestroyCommandPool(m_deviceContext->GetDevice(), m_immTransferPool,
                         nullptr);

  m_pipelineManager.Reset();
  m_resourcePool.Reset();
  m_swapchainContext.Reset();
  m_deviceContext.Reset();
}

auto VkRhi::OnLoad() -> EStatusCode { return EStatusCode::Success; };

auto VkRhi::Initialize(const DeviceRequirement &requirement,
                       const window::NativeWindowInfo &windowInfo,
                       uint32_t width, uint32_t height) -> ERhiResult {
  m_deviceContext = MakeUnique<DeviceContext>();
  auto config = TranslateRequirements(requirement);
  auto result = m_deviceContext->Initialize(config, windowInfo)
                    .and_then([&] -> std::expected<void, ERhiResult> {
                      if (!config.queueRequirement.isRequirePresent)
                        return {};
                      m_swapchainContext =
                          MakeUnique<SwapchainContext>(*m_deviceContext.Get());
                      return m_swapchainContext->Initialize(width, height);
                    })
                    .and_then([&]() { return CreateCommandPools(); })
                    .and_then([&]() { return CreateSyncObjects(); });
  if (!result.has_value())
    return result.error();

  m_resourcePool = MakeUnique<ResourcePool>(
      m_deviceContext->GetDevice(), m_deviceContext->GetPhysicalDevice());
  m_pipelineManager =
      MakeUnique<PipelineManager>(m_deviceContext->GetDevice(), *this);
  return {};
}

auto VkRhi::GetSwapchainImageFormat() -> EFormat {
  auto format = m_swapchainContext->GetImageFormat();
  switch (format) {
  case VkFormat::VK_FORMAT_B8G8R8_UNORM:
    return EFormat::R8G8B8_UNORM;
  case VkFormat::VK_FORMAT_R8G8B8_SRGB:
    return EFormat::R8G8B8_SRGB;
  case VkFormat::VK_FORMAT_B8G8R8A8_UNORM:
    return EFormat::R8G8B8A8_UNORM;
  case VkFormat::VK_FORMAT_R8G8B8A8_SRGB:
    return EFormat::R8G8B8A8_SRGB;
  case VkFormat::VK_FORMAT_B8G8R8A8_SRGB:
    return EFormat::B8G8R8A8_SRGB;
  default:
    return EFormat::Undefined;
  }
}

void VkRhi::SetSwapchainRenderPass(RenderPassHandle handle) {
  CreateSwapchianFrameBuffers(handle);
}

auto VkRhi::GetRenderPass(RenderPassHandle handle)
    -> const RenderPassResource & {
  return *m_resourcePool->ResolveRenderPass({handle.id});
}
auto VkRhi::GetFrameBuffer(ERenderTarget target)
    -> const FrameBufferResource & {
  switch (target) {
  case ERenderTarget::SwapchainBackBuffer:
    return *m_resourcePool->ResolveFrameBuffer(
        m_swapchainContext->GetFrameBuffer(m_currentImageIndex));
  }
}

auto VkRhi::GetPipeline(PipelineHandle handle) -> const PipelineResource & {
  return *m_pipelineManager->Resolve({handle.id});
}

auto VkRhi::GetBuffer(BufferHandle handle) -> const BufferResource & {
  return *m_resourcePool->ResolveBuffer({handle.id});
}

auto VkRhi::RecreateSwapchain(RenderPassHandle handle, uint32_t width,
                              uint32_t height) -> ERhiResult {
  vkDeviceWaitIdle(m_deviceContext->GetDevice());
  CleanupSwapchainFrameBuffers();
  auto result = m_swapchainContext->RecreateSwapchain(width, height);
  if (!result) {
    return result.error();
  }

  CreateSwapchianFrameBuffers(handle);

  // for (auto &syncObject : m_frameSyncObjects) {
  //   vkResetFences(m_context->GetDevice(), 1, &syncObject.m_inflightFence);
  // }

  return ERhiResult::Success;
}

auto VkRhi::CreateBuffer(const BufferCreateInfo &info) -> BufferHandle {
  return {m_resourcePool->CreateBuffer(info).id};
}

void VkRhi::ReleaseBuffer(BufferHandle handle) {
  m_resourcePool->ReleaseBuffer({handle.id});
}

auto VkRhi::CreatePipeline(const PipelineCreateInfo &info) -> PipelineHandle {
  return {m_pipelineManager->GetOrCreate(info).id};
}

auto VkRhi::CreateRenderPass(const RenderPassCreateInfo &info)
    -> RenderPassHandle {
  return {m_resourcePool->CreateRenderPass(info).id};
}

auto VkRhi::CreateCommandBuffer() -> ICommandBuffer * {
  VkCommandBufferAllocateInfo info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = m_frameCommandPools[m_currentFrame],
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  VkCommandBuffer vkCmdBuffer;
  auto result = vkAllocateCommandBuffers(m_deviceContext->GetDevice(), &info,
                                         &vkCmdBuffer);

  if (result != VK_SUCCESS) {
    Error("[Vulkan]: Failed to allocate command buffer! Error code: {}.",
          ToView(result));
    return nullptr;
  }
  auto cmdBuffer = MakeUnique<CommandBuffer>(vkCmdBuffer, *this);
  m_frameCommandBuffers.PushBack(std::move(cmdBuffer));

  return m_frameCommandBuffers.GetBack().Get();
}

void VkRhi::ExcuteOnce(EQueueType queueType,
                       const std::function<void(ICommandBuffer *)> &action) {

  auto pool = m_immTransferPool;
  auto device = m_deviceContext->GetDevice();
  vkResetCommandPool(device, pool, 0);

  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  VkCommandBuffer vkCmd;

  auto result = vkAllocateCommandBuffers(device, &allocInfo, &vkCmd);
  if (result != VK_SUCCESS) {
    Error("[Vulkan]: Failed to allocate command buffer! Error code: {}.",
          ToView(result));
    return;
  }

  auto cmd = MakeUnique<CommandBuffer>(vkCmd, *this);

  cmd->Begin();
  action(cmd.Get());
  cmd->End();

  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &vkCmd,
  };

  VkQueue queue = m_deviceContext->GetQueue(queueType);

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);
}

void *VkRhi::MapMemory(BufferHandle handle) {
  auto bufferResource = m_resourcePool->ResolveBuffer({handle.id});
  if (!bufferResource)
    return nullptr;

  void *mappedData = nullptr;

  auto result =
      vkMapMemory(m_deviceContext->GetDevice(), bufferResource->memory, 0,
                  bufferResource->size, 0, &mappedData);

  if (result != VK_SUCCESS) {
    Error("[Vulkan]: Failed to map memory! Error code: {}.", ToView(result));
    return nullptr;
  }

  return mappedData;
}
void VkRhi::UnmapMemory(BufferHandle handle) {
  auto bufferResource = m_resourcePool->ResolveBuffer({handle.id});
  if (bufferResource) {
    vkUnmapMemory(m_deviceContext->GetDevice(), bufferResource->memory);
  }
}

void VkRhi::Submit(ICommandBuffer *cmd) {
  auto *commandBuffer = static_cast<CommandBuffer *>(cmd);
  auto vkCmd = commandBuffer->GetRaw();
  VkPipelineStageFlags pipelineStageFlags[] = {
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &m_frameSyncObjects[m_currentFrame].imageAvailableSemaphore,
      .pWaitDstStageMask = pipelineStageFlags,
      .commandBufferCount = 1,
      .pCommandBuffers = &vkCmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &m_frameSyncObjects[m_currentImageIndex].renderFinishedSemaphore,
  };

  vkQueueSubmit(m_deviceContext->GetQueue(EQueueType::Graphics), 1, &info,
                m_frameSyncObjects[m_currentFrame].m_inflightFence);
}

auto VkRhi::BeginFrame() -> ERhiResult {
  AVALON_ASSERT(m_maxFrameInFlight == m_swapchainContext->GetImageCount());
  auto res = vkWaitForFences(
      m_deviceContext->GetDevice(), 1,
      &m_frameSyncObjects[m_currentFrame].m_inflightFence, VK_TRUE, UINT64_MAX);

  auto result = vkAcquireNextImageKHR(
      m_deviceContext->GetDevice(), m_swapchainContext->GetSwapchain(),
      UINT64_MAX, m_frameSyncObjects[m_currentFrame].imageAvailableSemaphore,
      VK_NULL_HANDLE, &m_currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    avalon::Warn("Vulkan: Swapchain is out of date!");
    return ERhiResult::SwapchainOutOfDate;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    avalon::Error("Vulkan: Failed to acquire swapchain image!");
    return HandleVkError(result);
  }

  vkResetFences(m_deviceContext->GetDevice(), 1,
                &m_frameSyncObjects[m_currentFrame].m_inflightFence);
  m_frameCommandBuffers.Clear();
  vkResetCommandPool(m_deviceContext->GetDevice(),
                     m_frameCommandPools[m_currentFrame], 0);

  return {};
}

auto VkRhi::EndFrame() -> ERhiResult {
  VkSwapchainKHR swapchains[] = {m_swapchainContext->GetSwapchain()};
  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &m_frameSyncObjects[m_currentImageIndex].renderFinishedSemaphore,
      .swapchainCount = 1,
      .pSwapchains = swapchains,
      .pImageIndices = &m_currentImageIndex,
  };

  auto result = vkQueuePresentKHR(
      m_deviceContext->GetQueue(EQueueType::Present), &presentInfo);

  auto ret = ERhiResult::Success;
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    ret = ERhiResult::SwapchainOutOfDate;
  } else if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to present queue!");
    return HandleVkError(result);
  }
  m_currentFrame = (m_currentFrame + 1) % m_maxFrameInFlight;
  return ret;
}

auto VkRhi::CreateCommandPools() -> std::expected<void, ERhiResult> {
  if (m_maxFrameInFlight == 0)
    m_maxFrameInFlight = m_swapchainContext->GetImageCount();
  m_frameCommandPools.Resize(m_maxFrameInFlight);

  auto indices = m_deviceContext->GetQueueFamilyIndices();

  VkCommandPoolCreateInfo graphicsPoolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = indices.graphicsFamily.value(),
  };

  for (auto &pool : m_frameCommandPools) {
    auto result = vkCreateCommandPool(m_deviceContext->GetDevice(),
                                      &graphicsPoolInfo, nullptr, &pool);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create command pool! Error code: {}.",
                    ToView(result));
      return std::unexpected(HandleVkError(result));
    }
  }

  auto queueFamilyIndex = indices.transferFamily.has_value()
                              ? indices.transferFamily.value()
                              : indices.graphicsFamily.value();

  VkCommandPoolCreateInfo transferPoolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
               VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamilyIndex,
  };

  auto result =
      vkCreateCommandPool(m_deviceContext->GetDevice(), &transferPoolInfo,
                          nullptr, &m_immTransferPool);

  if (result != VK_SUCCESS) {
    avalon::Error("Vulkan: Failed to create command pool! Error code: {}.",
                  ToView(result));
    return std::unexpected(HandleVkError(result));
  }

  return {};
}

void VkRhi::CleanupSwapchainFrameBuffers() {
  auto frameBuffers = m_swapchainContext->GetFrameBuffers();
  for (auto &fb : frameBuffers) {
    m_resourcePool->ReleaseFrameBuffer({fb.id});
  }
}

void VkRhi::CreateSwapchianFrameBuffers(RenderPassHandle handle) {
  auto extent = m_swapchainContext->GetExtent();
  auto frameBuffers = m_resourcePool->CreateSwapchainFrameBuffers(
      handle, m_swapchainContext->GetImageViews(), extent.width, extent.height,
      1);
  m_swapchainContext->SetFrameBuffers(std::move(frameBuffers));
}

auto VkRhi::CreateSyncObjects() -> std::expected<void, ERhiResult> {
  if (m_maxFrameInFlight == 0)
    m_maxFrameInFlight = m_swapchainContext->GetImageCount();
  VkSemaphoreCreateInfo semaphoreCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  VkFenceCreateInfo fenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  m_frameSyncObjects.Resize(m_maxFrameInFlight);
  for (auto &sync : m_frameSyncObjects) {
    auto result =
        vkCreateSemaphore(m_deviceContext->GetDevice(), &semaphoreCreateInfo,
                          nullptr, &sync.imageAvailableSemaphore);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create sync objects!");
      return std::unexpected(HandleVkError(result));
    }

    result =
        vkCreateSemaphore(m_deviceContext->GetDevice(), &semaphoreCreateInfo,
                          nullptr, &sync.renderFinishedSemaphore);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create sync objects!");
      return std::unexpected(HandleVkError(result));
    }

    result = vkCreateFence(m_deviceContext->GetDevice(), &fenceCreateInfo,
                           nullptr, &sync.m_inflightFence);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create sync objects!");
      return std::unexpected(HandleVkError(result));
    }
  }
  return {};
}
} // namespace avalon::rhi
