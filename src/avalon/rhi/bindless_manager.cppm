module;
#include <cstddef>
#include <cstdint>
export module avalon.rhi:bindless_manager;
import :types;
import avalon.core;
export namespace avalon::rhi {

constexpr uint32_t kNoMiplevels = -1;

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

constexpr uint32_t kRWTexturesBinding = 7;
constexpr uint32_t kComputeBufferBinding = 8;
constexpr uint32_t kRWTextureArraysBinding = 9;

constexpr uint32_t kMaxRWTextureDescriptor = 1024;
constexpr uint32_t kMaxRWTextureArrayDescriptor = 512;

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

constexpr uint32_t kMaxStorageImageDescriptor =
    kMaxRWTextureDescriptor + kMaxRWTextureArrayDescriptor;

constexpr size_t kDynamicSSBOSize = 1024 * 1024 * 64;

struct alignas(4) StandardMaterialData {
  Color albedo;
  float metallic;
  float roughness;
  float ao;
  float emissive;

  uint32_t albedoTex;
  uint32_t normalTex;
  uint32_t pbrTex;
  uint32_t sampler;
};

struct alignas(16) ProbeData {
  Matrix4x4 captureViews[6];
};

class IBindlessManager {
public:
  virtual uint32_t
  RegisterTexture(TextureHandle handle,
                  EResourceUsage usage = EResourceUsage::ReadOnly,
                  int32_t mipLevel = kNoMiplevels) = 0;
  virtual void UnregisterTexture(TextureHandle handle,
                                 int32_t mipLevel = kNoMiplevels) = 0;

  virtual uint32_t
  RegisterTextureArray(TextureHandle handle,
                       EResourceUsage usage = EResourceUsage::ReadOnly,
                       int32_t mipLevel = kNoMiplevels) = 0;
  virtual void UnregisterTextureArray(TextureHandle handle,
                                      int32_t mipLevel = kNoMiplevels) = 0;

  virtual uint32_t
  RegisterTextureCube(TextureHandle handle,
                      EResourceUsage usage = EResourceUsage::ReadOnly,
                      int32_t mipLevel = kNoMiplevels) = 0;
  virtual void UnregisterTextureCube(TextureHandle handle,
                                     int32_t mipLevel = kNoMiplevels) = 0;

  virtual uint32_t
  RegisterTexture3D(TextureHandle handle,
                    EResourceUsage usage = EResourceUsage::ReadOnly,
                    int32_t mipLevel = kNoMiplevels) = 0;
  virtual void UnregisterTexture3D(TextureHandle handle,
                                   int32_t mipLevel = kNoMiplevels) = 0;

  virtual uint32_t RegisterSampler(SamplerHandle) = 0;

  virtual uint32_t
  RegisterRWTexture(TextureHandle handle,
                    EResourceUsage usage = EResourceUsage::ReadOnly,
                    int32_t mipLevel = kNoMiplevels) = 0;
  virtual void UnregisterRWTexture(TextureHandle handle, int32_t mipLevel) = 0;

  virtual uint32_t
  RegisterRWTextureArray(TextureHandle handle,
                         EResourceUsage usage = EResourceUsage::ReadOnly,
                         int32_t mipLevel = kNoMiplevels) = 0;
  virtual void UnregisterRWTextureArray(TextureHandle handle,
                                        int32_t mipLevel) = 0;
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

  // Debug("{}", String::Format("  BaseColor:     {}, {}, {}, {}",
  //                            data->baseColor[0], data->baseColor[1],
  //                            data->baseColor[2], data->baseColor[3]));
  //
  // Debug("{}", String::Format("  SpecularColor: {}, {}, {}, {}",
  //                            data->specularColor[0], data->specularColor[1],
  //                            data->specularColor[2],
  //                            data->specularColor[3]));
  //
  // Debug("{}", String::Format("  Shininess:      {}", data->shininess));
  // Debug("{}", String::Format("  F0:             {}", data->f0));
  //
  // Debug("{}", String::Format("  Padding Check:  {}, {}", data->padding.x,
  //                            data->padding.y));

  Debug("============================");
}
} // namespace avalon::debug
#endif // !NDBUG
