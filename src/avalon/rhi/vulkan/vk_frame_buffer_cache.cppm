module;
#include <utility>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:frame_buffer_cache;
import avalon.core;
import avalon.rhi;
import :utils;
import :types;

namespace avalon::rhi {
class FrameBufferCache : public NonCopyable,
                         public mem::AutoDestroyable<FrameBufferCache> {
public:
  explicit FrameBufferCache(VkDevice device) : m_device(device) {}

  auto GetOrCreateFrameBuffer(const VkRenderPass pass,
                              const FrameBufferCreateInfo &createInfo)
      -> Handle<FrameBufferResource> {
    auto hash = createInfo.GetHash();
    if (m_cache.Contains(hash)) {
      return *m_cache.Get(hash);
    }

    auto handle = CreateFrameBuffer(pass, createInfo.views, createInfo.width,
                                    createInfo.height, createInfo.layers);
    m_cache.Insert(hash, handle);
    return handle;
  }

  auto Resolve(Handle<FrameBufferResource> handle) {
    return m_pool.Resolve(handle);
  }

  void Release(Handle<FrameBufferResource> handle) { m_pool.Release(handle); }

  template <std::invocable<RenderPassResource &> Func>
  void Foreach(Func &&func) {
    m_pool.Foreach(std::forward<Func>(func));
  }

  template <std::invocable<const RenderPassResource &> Func>
  void Foreach(Func &&func) const {
    m_pool.Foreach(std::forward<Func>(func));
  }

private:
  auto CreateFrameBuffer(VkRenderPass pass, const Array<VkImageView> &views,
                         uint32_t width, uint32_t height, uint32_t layers)
      -> Handle<FrameBufferResource> {

    VkFramebuffer frameBuffer;
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = pass,
        .attachmentCount = static_cast<uint32_t>(views.GetSize()),
        .pAttachments = views.GetData(),
        .width = width,
        .height = height,
        .layers = layers,
    };

    auto result =
        vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &frameBuffer);
    if (result != VK_SUCCESS) {
      HandleVkError(result);
      return {};
    }

    auto frameHandle = m_pool.Create(m_device, frameBuffer);
    return frameHandle;
  }

  VkDevice m_device;
  HashMap<HashType, Handle<FrameBufferResource>> m_cache;
  mem::ResourcePool<FrameBufferResource> m_pool;
};
} // namespace avalon::rhi
