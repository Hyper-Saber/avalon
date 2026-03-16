module;
export module avalon.rhi:descriptor_writer;

import :types;
import avalon.core;

export namespace avalon::rhi {

class IDescriptorWriter {
public:
  virtual ~IDescriptorWriter() = default;

  virtual bool IsValid() const = 0;
  virtual auto WriteBuffer(StringId id, const BufferWriteInfo &info)
      -> IDescriptorWriter & = 0;
  virtual auto WriteTexture(StringId id, TextureHandle texture,
                            SamplerHandle sampler) -> IDescriptorWriter & = 0;

  virtual auto Build() -> DescriptorSetHandle = 0;
};
} // namespace avalon::rhi
