module;
#include <utility>
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

    bool hasDepth = createInfo.depthAttachmentIndex != -1;
    auto depthAttachmentIndex = createInfo.depthAttachmentIndex;
    for (uint32_t i = 0; i < createInfo.attachments.GetSize(); i++) {
      const auto &src = createInfo.attachments[i];
      vkAttachments.PushBack({
          .format = ToVkFormat(createInfo.attachments[i].format),
          .samples = ToVkSampleCount(createInfo.attachments[i].sampleCount),
          .loadOp = ToVkLoadOp(src.loadOp),
          .storeOp = ToVkStoreOp(src.storeOp),
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = ToVkImageLayout(src.initialLayout),
          .finalLayout = ToVkImageLayout(src.finalLayout),
      });
      if (i == depthAttachmentIndex) {
        depthReference = {
            .attachment =
                static_cast<uint32_t>(createInfo.depthAttachmentIndex),
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
      } else
        colorReferences.PushBack({
            .attachment = i,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });

      Debug("[Vulkan]: Attachment {}: "
            "\n---------------------------------------\nintent: {}\nformat: "
            "{}\nloadOp: "
            "{}\nstoreOp: {}\ninitialLayout: {}\nfinalLayout: "
            "{}\n---------------------------------------",
            src.nameHash.Resolve(), ToView(src.intent), ToView(src.format),
            ToView(src.loadOp), ToView(src.storeOp), ToView(src.initialLayout),
            ToView(src.finalLayout));
    }

    VkSubpassDescription subpassDesc{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount =
            static_cast<uint32_t>(colorReferences.GetSize()),
        .pColorAttachments = colorReferences.GetData(),
        .pDepthStencilAttachment = hasDepth ? &depthReference : nullptr,
    };

    auto dependency = DeriveDependency(colorReferences.GetSize() > 0, hasDepth);

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

  template <std::invocable<RenderPassResource &> Func>
  void Foreach(Func &&func) {
    m_renderPassPool.Foreach(std::forward<Func>(func));
  }

  template <std::invocable<const RenderPassResource &> Func>
  void Foreach(Func &&func) const {
    m_renderPassPool.Foreach(std::forward<Func>(func));
  }

private:
  auto DeriveDependency(bool hasColor, bool hasDepth) -> VkSubpassDependency {
    VkSubpassDependency dep{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = 0,
        .dstStageMask = 0,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
    };

    if (hasColor) {
      dep.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dep.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dep.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (hasDepth) {
      dep.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dep.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dep.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    return dep;
  }

  VkDevice m_device;
  HashMap<HashType, Handle<RenderPassResource>> m_cache;
  mem::ResourcePool<RenderPassResource> m_renderPassPool;
};
} // namespace avalon::rhi
