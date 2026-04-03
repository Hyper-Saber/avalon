module;
#include <cstdint>
export module avalon.rhi:bindless_manager;
import :types;
import avalon.core;
export namespace avalon::rhi {

constexpr uint32_t kInternalSetCount = 2;

constexpr uint32_t kBindlessSet = 0;
constexpr uint32_t kSamplersBinding = 0;
constexpr uint32_t kMaterialsBinding = 1;
constexpr uint32_t kProbesBinding = 2;
constexpr uint32_t kTextureCubeBinding = 3;
constexpr uint32_t kTexture2DArrayBinding = 4;
constexpr uint32_t kTexture3DBinding = 5;
constexpr uint32_t kTexturesBinding = 6;
constexpr uint32_t kSceneGlobalsSet = 1;
constexpr uint32_t kSceneGlobalsBinding = 0;

constexpr uint32_t kMaxSamplerDescriptor = 256;

constexpr uint32_t kMaxMaterialCount = 65535; // uMaterials
constexpr uint32_t kMaxProbeCount = 128;      // uProbes

constexpr uint32_t kMaxTexture2DDescriptor = 1024 * 16;
constexpr uint32_t kMaxTextureCubeDescriptor = 128;
constexpr uint32_t kMaxTextureArrayDescriptor = 128;
constexpr uint32_t kMaxTexture3DDescriptor = 128;

constexpr uint32_t kMaxSampledImageDescriptor =
    kMaxTexture2DDescriptor + kMaxTextureCubeDescriptor +
    kMaxTextureArrayDescriptor + kMaxTexture3DDescriptor;

constexpr uint32_t kMaxStorageImageDescriptor = 1024;

struct alignas(4) StandardMaterialData {
  float baseColor[4];
  float specularColor[4];
  float shininess;
  float f0;
  Vec2 padding;
};

struct alignas(16) ProbeData {
  Matrix4x4 captureViews[6];
};

class IBindlessManager {
public:
  virtual uint32_t RegisterTexture(TextureHandle handle) = 0;
  virtual void UnregisterTexture(TextureHandle handle) = 0;
  virtual uint32_t RegisterTextureArray(TextureHandle handle) = 0;
  virtual void UnregisterTextureArray(TextureHandle handle) = 0;
  virtual uint32_t RegisterTextureCube(TextureHandle handle) = 0;
  virtual void UnregisterTextureCube(TextureHandle handle) = 0;
  virtual uint32_t RegisterTexture3D(TextureHandle handle) = 0;
  virtual void UnregisterTexture3D(TextureHandle handle) = 0;
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
