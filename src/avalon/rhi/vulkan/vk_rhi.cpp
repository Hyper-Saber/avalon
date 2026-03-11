module;
#include <debug/assert.hpp>
#include <expected>
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
  vkDeviceWaitIdle(m_context->GetDevice());
  for (auto &syncObject : m_frameSyncObjects) {
    if (syncObject.renderFinishedSemaphore != VK_NULL_HANDLE)
      vkDestroySemaphore(m_context->GetDevice(),
                         syncObject.renderFinishedSemaphore, nullptr);
    if (syncObject.imageAvailableSemaphore != VK_NULL_HANDLE)
      vkDestroySemaphore(m_context->GetDevice(),
                         syncObject.imageAvailableSemaphore, nullptr);
    if (syncObject.m_inflightFence != VK_NULL_HANDLE)
      vkDestroyFence(m_context->GetDevice(), syncObject.m_inflightFence,
                     nullptr);
  }

  for (auto &pool : m_commandPools) {
    if (pool != VK_NULL_HANDLE)
      vkDestroyCommandPool(m_context->GetDevice(), pool, nullptr);
  }

  m_pipelineManager.Reset();
  m_resourcePool.Reset();
  m_swapchainContext.Reset();
  m_context.Reset();
}

auto VkRhi::OnLoad() -> EStatusCode { return EStatusCode::Success; };

auto VkRhi::Initialize(const DeviceRequirement &requirement,
                       const window::NativeWindowInfo &windowInfo,
                       uint32_t width, uint32_t height) -> ERhiResult {
  m_context = MakeUnique<DeviceContext>();
  auto config = TranslateRequirements(requirement);
  auto result = m_context->Initialize(config, windowInfo)
                    .and_then([&] -> std::expected<void, ERhiResult> {
                      if (!config.queueRequirement.isRequirePresent)
                        return {};
                      m_swapchainContext =
                          MakeUnique<SwapchainContext>(*m_context.Get());
                      return m_swapchainContext->Initialize(width, height);
                    })
                    .and_then([&]() { return CreateCommandPools(); })
                    .and_then([&]() { return CreateSyncObjects(); });
  if (!result.has_value())
    return result.error();

  m_resourcePool = MakeUnique<ResourcePool>(m_context->GetDevice(),
                                            m_context->GetPhysicalDevice());
  m_pipelineManager =
      MakeUnique<PipelineManager>(m_context->GetDevice(), *this);
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
  vkDeviceWaitIdle(m_context->GetDevice());
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
      .commandPool = m_commandPools[m_currentFrame],
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  VkCommandBuffer vkCmdBuffer;
  auto result =
      vkAllocateCommandBuffers(m_context->GetDevice(), &info, &vkCmdBuffer);

  if (result != VK_SUCCESS) {
    Error("[Vulkan]: Failed to allocate command buffer! Error code: {}.",
          ToView(result));
    return nullptr;
  }
  auto cmdBuffer = MakeUnique<CommandBuffer>(vkCmdBuffer, *this);
  m_commandBuffers.PushBack(std::move(cmdBuffer));

  return m_commandBuffers.GetBack().Get();
}

void VkRhi::SetSwapchainRenderPass(RenderPassHandle handle) {
  CreateSwapchianFrameBuffers(handle);
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

  vkQueueSubmit(m_context->GetGraphicsQueue(), 1, &info,
                m_frameSyncObjects[m_currentFrame].m_inflightFence);
}

auto VkRhi::BeginFrame() -> ERhiResult {
  AVALON_ASSERT(m_maxFrameInFlight == m_swapchainContext->GetImageCount());
  auto res = vkWaitForFences(
      m_context->GetDevice(), 1,
      &m_frameSyncObjects[m_currentFrame].m_inflightFence, VK_TRUE, UINT64_MAX);

  auto result = vkAcquireNextImageKHR(
      m_context->GetDevice(), m_swapchainContext->GetSwapchain(), UINT64_MAX,
      m_frameSyncObjects[m_currentFrame].imageAvailableSemaphore,
      VK_NULL_HANDLE, &m_currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    avalon::Warn("Vulkan: Swapchain is out of date!");
    return ERhiResult::SwapchainOutOfDate;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    avalon::Error("Vulkan: Failed to acquire swapchain image!");
    return HandleVkError(result);
  }

  vkResetFences(m_context->GetDevice(), 1,
                &m_frameSyncObjects[m_currentFrame].m_inflightFence);
  m_commandBuffers.Clear();
  vkResetCommandPool(m_context->GetDevice(), m_commandPools[m_currentFrame], 0);

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

  auto result = vkQueuePresentKHR(m_context->GetPresentQueue(), &presentInfo);

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
  m_commandPools.Resize(m_maxFrameInFlight);
  auto indices = m_context->GetQueueFamilyIndices();
  VkCommandPoolCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = indices.graphicsFamily.value(),
  };

  for (auto &pool : m_commandPools) {
    auto result = vkCreateCommandPool(m_context->GetDevice(), &createInfo,
                                      nullptr, &pool);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create command pool! Error code: {}.",
                    ToView(result));
      return std::unexpected(HandleVkError(result));
    }
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
        vkCreateSemaphore(m_context->GetDevice(), &semaphoreCreateInfo, nullptr,
                          &sync.imageAvailableSemaphore);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create sync objects!");
      return std::unexpected(HandleVkError(result));
    }

    result = vkCreateSemaphore(m_context->GetDevice(), &semaphoreCreateInfo,
                               nullptr, &sync.renderFinishedSemaphore);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create sync objects!");
      return std::unexpected(HandleVkError(result));
    }

    result = vkCreateFence(m_context->GetDevice(), &fenceCreateInfo, nullptr,
                           &sync.m_inflightFence);
    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create sync objects!");
      return std::unexpected(HandleVkError(result));
    }
  }
  return {};
}
} // namespace avalon::rhi
