module;
#include <cstring>
#include <debug/assert.hpp>
#include <expected>
#include <functional>
#include <optional>
#include <vulkan/vulkan.h>

module avalon.rhi.vulkan;

import avalon.core;
import avalon.rhi;
import :utils;
import :descriptor_writer;
import :descriptor_provider;

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

  m_descriptorProvider.Reset();
  m_bindlessManager.Reset();
  m_materialPool.Reset();
  m_staticPool.Reset();
  m_geometryPool.Reset();
  m_attributesPool.Reset();
  m_indicesPool.Reset();
  m_uboPool.Reset();
  m_dynamicPool.Reset();
  m_indirectPool.Reset();
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
                    .and_then([&]() {
                      m_stateTracker = MakeUnique<StateTracker>();
                      return CreateCommandPools();
                    })
                    .and_then([&]() { return CreateSyncObjects(); });
  if (!result.has_value())
    return result.error();

  m_resourcePool = MakeUnique<ResourcePool>(*m_deviceContext.Get());
  CreateUBOPool();

  auto usage = EResourceUsage::StorageBuffer | EResourceUsage::TransferSrc |
               EResourceUsage::TransferDst;

  CreateDynamicSSBOPool(m_dynamicPool, m_dynamicSSBODescriptorInfo,
                        kDynamicSSBOSize, usage);

  CreateDynamicSSBOPool(
      m_indirectPool, m_indirectSSBODescriptorInfo, kIndirectSSBOSize,
      EResourceUsage::StorageBuffer | EResourceUsage::IndirectBuffer);

  CreateStaticSSBOPool(m_materialPool, m_materialSSBODescriptorInfo,
                       kMaxMaterialCount * sizeof(StandardMaterialData), usage);
  CreateStaticSSBOPool(m_staticPool, m_staticSSBODescriptorInfo,
                       kStaticSSBOSize, usage);
  CreateStaticSSBOPool(m_geometryPool, m_geometrySSBOescriptorInfo,
                       kGeomtriesSSBOSize, usage);
  CreateStaticSSBOPool(m_attributesPool, m_attributesSSBODescriptorInfo,
                       kAttributesSSBOSize, usage);
  CreateStaticSSBOPool(
      m_indicesPool, m_indicesSSBODescriptorInfo, kIndicesSSBOSize,
      EResourceUsage::StorageBuffer | EResourceUsage::IndexBuffer);

  WarpSwapchainTextures();
  m_descriptorProvider =
      MakeUnique<DescriptorProvider>(m_deviceContext->GetDevice());
  m_bindlessManager = MakeUnique<BindlessManager>(
      m_deviceContext->GetDevice(), *this, *m_descriptorProvider.Get());
  m_pipelineManager =
      MakeUnique<PipelineManager>(m_deviceContext->GetDevice(), *this);
  CreateStaticSamplers();
  m_sceneGlobalSetWriter =
      MakeUnique<DescriptorWriter>(m_deviceContext->GetDevice(), *this);
  return {};
}

void VkRhi::CreateUBOPool() {
  m_uboPool = MakeUnique<rhi::RingBufferPool>(
      *this,
      EResourceUsage::UniformBuffer | EResourceUsage::TransferSrc |
          EResourceUsage::TransferDst,
      EMemoryProperty::DeviceLocal | EMemoryProperty::HostVisible |
          EMemoryProperty::HostCoherent,
      1024 * 1024 * 16);
}

void VkRhi::CreateDynamicSSBOPool(UniquePtr<RingBufferPool> &outUP,
                                  VkDescriptorBufferInfo &outInfo, size_t size,
                                  EResourceUsage usage) {

  outUP = MakeUnique<rhi::RingBufferPool>(*this, usage,
                                          EMemoryProperty::DeviceLocal |
                                              EMemoryProperty::HostVisible |
                                              EMemoryProperty::HostCoherent,
                                          size);

  auto pRes = m_resourcePool->ResolveBuffer({outUP->GetBufferHandle().id});
  outInfo = {
      .buffer = pRes->buffer,
      .offset = 0,
      .range = pRes->size,
  };
}

void VkRhi::CreateStaticSSBOPool(UniquePtr<LinearBufferPool> &outUP,
                                 VkDescriptorBufferInfo &outInfo, size_t size,
                                 EResourceUsage usage) {
  outUP = MakeUnique<rhi::LinearBufferPool>(*this, usage,
                                            EMemoryProperty::DeviceLocal |
                                                EMemoryProperty::HostVisible |
                                                EMemoryProperty::HostCoherent,
                                            size);

  auto pRes = m_resourcePool->ResolveBuffer({outUP->GetBufferHandle().id});
  outInfo = {
      .buffer = pRes->buffer,
      .offset = 0,
      .range = pRes->size,
  };
}

auto VkRhi::GetUBOPool() const -> RingBufferPool & { return *m_uboPool.Get(); }

auto VkRhi::GetDynamicSSBOPool() const -> RingBufferPool & {
  return *m_dynamicPool.Get();
}

auto VkRhi::GetBindlessManager() const -> IBindlessManager & {
  return *m_bindlessManager.Get();
}

auto VkRhi::GetIndexBuffer() const -> BufferHandle {
  return m_indicesPool->GetBufferHandle();
}

auto VkRhi::GetStaticSamplers() const -> const StaticSamplers & {
  return m_staticSamplers;
}

auto VkRhi::GetSwapchainImageFormat() const -> EFormat {
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

auto VkRhi::GetSwapchainExtent() const -> Extent2D {
  auto extent = m_swapchainContext->GetExtent();
  return {extent.width, extent.height};
}

auto VkRhi::GetCurrentPresentTexture() -> TextureHandle {
  return m_swapchainTextures[m_currentImageIndex];
}

auto VkRhi::GetMainCommandBuffer() const -> ICommandBuffer * {
  return m_frameCommandBuffers[m_currentFrame].Get();
}

uint32_t VkRhi::GetCurrentFrameIndex() const { return m_currentFrame; }
uint32_t VkRhi::GetMaxFrameInFlight() const { return m_maxFrameInFlight; }

auto VkRhi::GetCapabilities() const -> DeviceCapabilities {
  return m_deviceContext->GetCapabilities();
}

auto VkRhi::GetDefaultTexture() const -> TextureHandle {
  return m_defaultTexture;
}

auto VkRhi::GetPipeline(PipelineHandle handle) -> const PipelineResource * {
  return m_pipelineManager->Resolve({handle.id});
}

auto VkRhi::GetBuffer(BufferHandle handle) -> const BufferResource * {
  return m_resourcePool->ResolveBuffer({handle.id});
}

auto VkRhi::GetTexture(TextureHandle handle) -> const TextureResource * {
  return m_resourcePool->ResolveTexture({handle.id});
}

auto VkRhi::GetOrCreateMipStorageView(TextureHandle handle, uint32_t mipLevel)
    -> VkImageView {
  return m_resourcePool->GetOrCreateMipStorageView({handle.id}, mipLevel);
}

auto VkRhi::GetOrCreateDepthTextureView(TextureHandle handle) -> VkImageView {
  return m_resourcePool->GetOrCreateDepthTextureView({handle.id});
}

auto VkRhi::GetDummyComputePipeline() const -> PipelineHandle {
  return m_dummyComputePipeline;
}

auto VkRhi::GetSampler(SamplerHandle handle) -> const SamplerResource * {
  return m_resourcePool->ResolveSampler({handle.id});
}

auto VkRhi::GetDescriptorSet(DescriptorSetHandle handle)
    -> const DescriptorSetResource * {
  return m_descriptorProvider->Resolve(handle);
}

auto VkRhi::GetBindlessSet() const -> VkDescriptorSet {
  return m_bindlessManager->GetBindlessSet();
}

auto VkRhi::GetSceneGlobalSet() const -> VkDescriptorSet {
  return m_bindlessManager->GetSceneGlobalSet();
}

auto VkRhi::GetSceneGlobalSetHandle() const -> DescriptorSetHandle {
  return m_bindlessManager->GetSceneGlobalSetHandle();
}

auto VkRhi::GetBindlessSetLayout() const -> VkDescriptorSetLayout {
  return m_bindlessManager->GetBindlessSetLayout();
}

auto VkRhi::GetSceneGlobalSetLayout() const -> VkDescriptorSetLayout {
  return m_bindlessManager->GetSceneGlobalSetLayout();
}

auto VkRhi::GetCurrentSwapchainImage() const -> VkImage {
  return m_swapchainContext->GetImage(m_currentImageIndex);
}

auto VkRhi::GetTextureCreateInfo(TextureHandle handle) const
    -> TextureCreateInfo {
  return m_resourcePool->ResolveTexture({handle.id})->createInfo;
}

uint32_t VkRhi::GetCurrentFrameIndex() { return m_currentFrame; }
uint32_t VkRhi::GetLastCompletedFrameIndex() {
  return m_currentFrame - 1 < 0 ? m_maxFrameInFlight - 1 : m_currentFrame - 1;
}

auto VkRhi::GetMaterialSSBOInfo() const -> const VkDescriptorBufferInfo & {
  return m_materialSSBODescriptorInfo;
}

auto VkRhi::GetIndirectSSBOInfo() const -> const VkDescriptorBufferInfo & {
  return m_indirectSSBODescriptorInfo;
}

auto VkRhi::GetStaticSSBOInfo() const -> const VkDescriptorBufferInfo & {
  return m_staticSSBODescriptorInfo;
}

auto VkRhi::GetDynamicSSBOInfo() const -> const VkDescriptorBufferInfo & {
  return m_dynamicSSBODescriptorInfo;
}

auto VkRhi::GetGeometriesSSBOInfo() const -> const VkDescriptorBufferInfo & {
  return m_geometrySSBOescriptorInfo;
}

auto VkRhi::GetAttributesSSBOInfo() const -> const VkDescriptorBufferInfo & {
  return m_attributesSSBODescriptorInfo;
}

auto VkRhi::GetIndicesSSBOInfo() const -> const VkDescriptorBufferInfo & {
  return m_indicesSSBODescriptorInfo;
}

void VkRhi::UpdateMaterialBuffer(size_t offset, const void *data, size_t size) {
  m_materialPool->UpdateData(offset, data, size);
}

auto VkRhi::AllocateIndirectSSBO(size_t size) -> BufferAllocation {
  return m_indirectPool->AllocateAligned(size);
}

auto VkRhi::AllocateStaticSSBO(size_t size) -> BufferAllocation {
  return m_staticPool->AllocateAligned(size);
}

auto VkRhi::AllocateVertexGeometrySSBO(size_t size) -> BufferAllocation {
  return m_geometryPool->AllocateAligned(size);
}

auto VkRhi::AllocateVertexAttributesSSBO(size_t size) -> BufferAllocation {
  return m_attributesPool->AllocateAligned(size);
}

auto VkRhi::AllocateVertexIndicesSSBO(size_t size) -> BufferAllocation {
  return m_indicesPool->AllocateAligned(size);
}

auto VkRhi::RecreateSwapchain(uint32_t width, uint32_t height) -> ERhiResult {
  vkDeviceWaitIdle(m_deviceContext->GetDevice());
  auto result = m_swapchainContext->RecreateSwapchain(width, height);
  if (!result) {
    return result.error();
  }

  WarpSwapchainTextures();

  return ERhiResult::Success;
}

void VkRhi::WarpSwapchainTextures() {
  auto extent = m_swapchainContext->GetExtent();
  auto &views = m_swapchainContext->GetImageViews();
  auto &images = m_swapchainContext->GetImages();

  m_swapchainTextures.Clear();
  auto size = images.GetSize();
  m_swapchainTextures.Reserve(size);
  TextureCreateInfo info{
      .width = extent.width,
      .height = extent.height,
      .layerCount = 1,
      .usage = EResourceUsage::Present,
  };

  for (uint32_t i = 0; i < size; i++) {
    auto handle = m_resourcePool->ImportExternalTexture(
        m_deviceContext->GetDevice(), images[i], views[i], info, true);
    m_swapchainTextures.PushBack({handle.id});
  }
}

auto VkRhi::CreateBuffer(const BufferCreateInfo &info) -> BufferHandle {
  return {m_resourcePool->CreateBuffer(info).id};
}

void VkRhi::ReleaseBuffer(BufferHandle handle) {
  m_resourcePool->ReleaseBuffer({handle.id});
}

auto VkRhi::CreateTexture(const TextureCreateInfo &info) -> TextureHandle {
  return {m_resourcePool->CreateTexture(info).id};
}

void VkRhi::ReleaseTexture(TextureHandle handle) {
  m_bindlessManager->UnregisterTexture(handle);

  return m_resourcePool->ReleaseTexture({handle.id});
}

auto VkRhi::GetSceneGlobalSetWriter() -> IDescriptorWriter & {
  return *m_sceneGlobalSetWriter.Get();
}

auto VkRhi::CreateDescriptorWriter(PipelineHandle handle, uint32_t set)
    -> IDescriptorWriter & {
  m_descriptorWriter =
      MakeUnique<DescriptorWriter>(m_deviceContext->GetDevice(), *this,
                                   *m_descriptorProvider.Get(), handle, set);
  return *m_descriptorWriter.Get();
}

auto VkRhi::GetOrCreatePipeline(const PipelineCreateInfo &info)
    -> PipelineHandle {
  return {m_pipelineManager->GetOrCreate(info).id};
}

auto VkRhi::GetOrCreateComputePipeline(const ComputePipelineCreateInfo &info)
    -> PipelineHandle {
  PipelineHandle handle = {m_pipelineManager->GetOrCreate(info).id};
  if (!m_dummyComputePipeline.IsValid()) [[unlikely]]
    m_dummyComputePipeline = handle;
  return handle;
}

void VkRhi::CreateCommandBuffer() {
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
    return;
  }
  auto cmdBuffer =
      MakeUnique<CommandBuffer>(vkCmdBuffer, *this, *m_stateTracker.Get());
  m_frameCommandBuffers.PushBack(std::move(cmdBuffer));
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

  auto cmd = MakeUnique<CommandBuffer>(vkCmd, *this, *m_stateTracker.Get());

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

void VkRhi::Submit(ICommandBuffer &cmd) {
  auto &commandBuffer = static_cast<CommandBuffer &>(cmd);

  auto vkCmd = commandBuffer.GetRaw();
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

  m_stateTracker->Clear();
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
  vkResetCommandPool(m_deviceContext->GetDevice(),
                     m_frameCommandPools[m_currentFrame], 0);

  m_uboPool->ResetPool();
  m_dynamicPool->ResetPool();
  m_indirectPool->ResetPool();
  m_descriptorProvider->Flip();
  m_bindlessManager->ProcessPendingDeletions();
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

void VkRhi::WaitIdle() { vkDeviceWaitIdle(m_deviceContext->GetDevice()); }

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

    CreateCommandBuffer();
    m_currentFrame++;
  }

  m_currentFrame = 0;

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

void VkRhi::CreateStaticSamplers() {
  SamplerCreateInfo linearClampInfo{
      .magFilter = EFilter::Linear,
      .minFilter = EFilter::Linear,
      .mipmapMode = EMipmapMode::Linear,
      .addressModeU = EAddressMode::ClampToEdge,
      .addressModeV = EAddressMode::ClampToEdge,
      .addressModeW = EAddressMode::ClampToEdge,
  };

  auto handle = m_resourcePool->CreateSampler(linearClampInfo);
  m_staticSamplers.linearClamp =
      m_bindlessManager->RegisterSampler({handle.id});

  SamplerCreateInfo pointClampInfo{
      .magFilter = EFilter::Nearest,
      .minFilter = EFilter::Nearest,
      .mipmapMode = EMipmapMode::Nearest,
      .addressModeU = EAddressMode::ClampToEdge,
      .addressModeV = EAddressMode::ClampToEdge,
      .addressModeW = EAddressMode::ClampToEdge,
  };

  handle = m_resourcePool->CreateSampler(pointClampInfo);
  m_staticSamplers.pointClamp = m_bindlessManager->RegisterSampler({handle.id});
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

//-------------------------------DEBUG-------------------------------------
#ifndef NDEBUG

#endif // !NDEBUG
} // namespace avalon::rhi
