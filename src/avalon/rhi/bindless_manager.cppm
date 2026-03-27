module;
#include <cstdint>
export module avalon.rhi:bindless_manager;
import :types;
import avalon.core;
export namespace avalon::rhi {

struct alignas(16) StandardMaterialData {
  float baseColor[4];
  float specularColor[4];
  float shininess;
  float f0;
  Vec2 padding;
};

class IBindlessManager {
public:
  virtual uint32_t RegisterTexture(TextureHandle handle) = 0;
  virtual void UnregisterTexture(TextureHandle handle) = 0;
  virtual uint32_t RegisterSampler(SamplerHandle) = 0;
};
} // namespace avalon::rhi

//
//
//
#ifndef NDBUG

export namespace avalon::debug {
void DumpMaterial(void *pData) {
  auto *data = reinterpret_cast<rhi::StandardMaterialData *>(pData);

  Debug("=== Material Dump ===");

  Debug("{}", String::Format("  BaseColor:     {}, {}, {}, {}",
                             data->baseColor[0], data->baseColor[1],
                             data->baseColor[2], data->baseColor[3]));

  Debug("{}", String::Format("  SpecularColor: {}, {}, {}, {}",
                             data->specularColor[0], data->specularColor[1],
                             data->specularColor[2], data->specularColor[3]));

  Debug("{}", String::Format("  Shininess:      {}", data->shininess));
  Debug("{}", String::Format("  F0:             {}", data->f0));

  // 检查 Padding 是否为 0 (如果写入正确，这里应该是 0)
  Debug("{}", String::Format("  Padding Check:  {}, {}", data->padding.x,
                             data->padding.y));

  Debug("============================");
}
} // namespace avalon::debug
#endif // !NDBUG
