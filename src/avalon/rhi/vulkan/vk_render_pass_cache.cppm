module;
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:render_pass_cache;
import avalon.core;
import avalon.rhi;
import :utils;
import :types;

namespace avalon::rhi {
class RenderPassCache : public NonCopyable,
                        public mem::AutoDestroyable<RenderPassCache> {
public:
  explicit RenderPassCache(VkDevice device) : m_device(device) {}

  auto GetOrCreateRenderPass(const RenderPassCreateInfo &createInfo)
      -> Handle<RenderPassResource> {
    auto hash = createInfo.GetHash();
    if (m_cache.Contains(hash)) {
      return *m_cache.Get(hash);
    }

    Array<VkAttachmentDescription> vkAttachments;
    Array<VkAttachmentReference> colorReferences;
    VkAttachmentReference depthReference{};

    for (uint32_t i = 0; i < createInfo.colorAttachments.GetSize(); i++) {
      const auto &src = createInfo.colorAttachments[i];
      vkAttachments.PushBack({
          .format = ToVkFormat(createInfo.colorAttachments[i].format),
          .samples = ToVkSampleCount(createInfo.colorAttachments[i].sampleCount),
          .loadOp = ToVkLoadOp(src.loadOp),
          .storeOp = ToVkStoreOp(src.storeOp),
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = ToVkImageLayout(src.initialLayout),
          .finalLayout = ToVkImageLayout(src.finalLayout),
      });
      colorReferences.PushBack({
          .attachment = i,
          .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      });

      Debug("[Vulkan]: Color Attachment {}: "
            "\n---------------------------------------\nformat: {}\nloadOp: "
            "{}\nstoreOp: {}\ninitialLayout: {}\nfinalLayout: "
            "{}\n---------------------------------------",
            i, ToView(src.format), ToView(src.loadOp), ToView(src.storeOp),
            ToView(src.initialLayout), ToView(src.finalLayout));
    }

    if (createInfo.hasDepth) {
      uint32_t depthIndex = static_cast<uint32_t>(vkAttachments.GetSize());
      vkAttachments.PushBack({
          .format = ToVkFormat(createInfo.depthAttachment.format),
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = ToVkLoadOp(createInfo.depthAttachment.loadOp),
          .storeOp = ToVkStoreOp(createInfo.depthAttachment.storeOp),
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout =
              ToVkImageLayout(createInfo.depthAttachment.initialLayout),
          .finalLayout =
              ToVkImageLayout(createInfo.depthAttachment.finalLayout),
      });

      Debug("[Vulkan]: Depth Attachment: "
            "\n---------------------------------------\nformat: {}\nloadOp: "
            "{}\nstoreOp: {}\ninitialLayout: {}\nfinalLayout: "
            "{}\n---------------------------------------",
            ToView(createInfo.depthAttachment.format),
            ToView(createInfo.depthAttachment.loadOp),
            ToView(createInfo.depthAttachment.storeOp),
            ToView(createInfo.depthAttachment.initialLayout),
            ToView(createInfo.depthAttachment.finalLayout));

      depthReference = {
          .attachment = depthIndex,
          .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      };
    }

    VkSubpassDescription subpassDesc{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount =
            static_cast<uint32_t>(colorReferences.GetSize()),
        .pColorAttachments = colorReferences.GetData(),
        .pDepthStencilAttachment =
            createInfo.hasDepth ? &depthReference : nullptr,
    };

    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo vkPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(vkAttachments.GetSize()),
        .pAttachments = vkAttachments.GetData(),
        .subpassCount = 1,
        .pSubpasses = &subpassDesc,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VkRenderPass pass{VK_NULL_HANDLE};
    auto result =
        vkCreateRenderPass(m_device, &vkPassCreateInfo, nullptr, &pass);

    if (result != VK_SUCCESS) {
      avalon::Error("Vulkan: Failed to create shader module! Error code: {}",
                    ToView(result));
      return {};
    }

    auto handle = m_renderPassPool.Create(m_device, pass, createInfo);

    m_cache.Insert(hash, handle);
    return handle;
  }

  auto Resolve(Handle<RenderPassResource> handle) {
    return m_renderPassPool.Resolve(handle);
  }

private:
  VkDevice m_device;
  HashMap<HashType, Handle<RenderPassResource>> m_cache;
  mem::ResourcePool<RenderPassResource> m_renderPassPool;
};
} // namespace avalon::rhi
