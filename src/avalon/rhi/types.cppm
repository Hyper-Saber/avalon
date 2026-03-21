module;
#include <bit>
#include <cstdint>
#include <cstring>
#include <type_traits>

export module avalon.rhi:types;

import avalon.core;

export namespace avalon::rhi {

using BufferHandle = Handle<class BufferTag>;
using TextureHandle = Handle<class TextureTag>;
using SamplerHandle = Handle<class SamplerTag>;
using PipelineHandle = Handle<class PipelineTag>;
using RenderPassHandle = Handle<class RenderPassTag>;
using FrameBufferHandle = Handle<class FramebufferTag>;
using DescriptorSetHandle = Handle<class DescriptorSetTag>;

enum class EFormat {
  Undefined,
  R16_Uint,
  R16_Int,
  R16_Float,
  R16G16_Uint,
  R16G16_Int,
  R16G16_Float,
  R16G16B16_Uint,
  R16G16B16_Int,
  R16G16B16_Float,
  R16G16B16A16_Uint,
  R16G16B16A16_Int,
  R16G16B16A16_Float,
  R32_Uint,
  R32_Int,
  R32_Float,
  R32G32_Uint,
  R32G32_Int,
  R32G32_Float,
  R32G32B32_Uint,
  R32G32B32_Int,
  R32G32B32_Float,
  R32G32B32A32_Uint,
  R32G32B32A32_Int,
  R32G32B32A32_Float,
  R64_Uint,
  R64_Int,
  R64_Float,
  R64G64_Uint,
  R64G64_Int,
  R64G64_Float,
  R64G64B64_Uint,
  R64G64B64_Int,
  R64G64B64_Float,
  R64G64B64A64_Uint,
  R64G64B64A64_Int,
  R64G64B64A64_Float,

  R8G8B8_UNORM,
  R8G8B8A8_UNORM,
  R8G8B8_SRGB,
  R8G8B8A8_SRGB,
  B8G8R8A8_SRGB,
  R16G16B16A16_SFLOAT,
  D32_SFLOAT,
  D32_SFLOAT_S8_UINT,
};

enum class EImageFormat {

};

enum class EVertexSemantic {
  Unknown,
  Position,
  TexCoord,
  Color,
  Normal,
};

enum class EShaderStage : uint32_t {
  None,
  Vertex = 1 << 0,
  Fragment = 1 << 1,
  Compute = 1 << 2,
  All = Vertex | Fragment | Compute
};

enum class EShaderFeatureLevel : uint32_t {
  Level_6_0,
  Level_6_3,
  Level_6_6,
  Default
};

enum class EPrimitiveTopology : uint32_t {
  PointList,
  LineList,
  TriangleList,
};

enum class EPolygonMode : uint32_t { Fill, Line, Point };
enum class ECullMode : uint32_t { Back, Front, FrontAndBack, None };
enum class ECompareOp : uint32_t {
  Less,
  LessOrEqual,
  Greater,
  GreaterOrEqual,
  Equal,
  NotEqual
};

enum class EBufferUsage : uint32_t {
  None = 0,
  Vertex = 1 << 0,
  Index = 1 << 1,
  Uniform = 1 << 2,
  Storage = 1 << 3,
  Indirect = 1 << 4,
  TransferSrc = 1 << 5,
  TransferDst = 1 << 6
};

enum class EDescriptorType : uint32_t {
  UniformBuffer,
  StorageBuffer,
  UniformBufferDynamic,
  CombinedImageSampler,
  SampledImage,
  Sampler,
  StorageImage,
  UniformTexelBuffer,
  StorageTexelBuffer,
  StorageBufferDynamic,
  InputAttachment,
  AccelerationStructure
};

enum class EAttachmentLoadOp { Load, Clear, DontCare };
enum class EAttachmentStoreOp { Store, DontCare };
enum class EMemoryProperty : uint32_t {
  None = 0,
  DeviceLocal = 1 << 0,
  HostVisible = 1 << 1,
  HostCoherent = 1 << 2,
  All = DeviceLocal | HostVisible | HostCoherent,
};

enum class EResourceLayout {
  Undefined,
  ColorAttachment,
  DepthStencilAttachment,
  ShaderReadOnly,
  Present,
  TransferSrc,
  TransferDst,
};

enum class ESampleCount {
  SampleCount1x,
  SampleCount2x,
  SampleCount4x,
  SampleCount8x,
  SampleCount16x,
};

enum class ETextureUsage : uint32_t {
  None = 0,
  Sampled = 1 << 0,
  ColorAttachment = 1 << 1,
  DepthStencilAttachment = 1 << 2,
  Storage = 1 << 3,
  TransferSrc = 1 << 4,
};

enum class EAttachmentIntent : uint32_t {
  None = 0,
  ReadOnly = 1 << 0,
  WriteColor = 1 << 1,
  WriteDepth = 1 << 2,
  CaptureSource = 1 << 3,
  ComputeStorage = 1 << 4,
};

enum class EQueueType { Graphics, Transfer, Compute, Present };

enum class ERenderCapability : uint32_t {
  Swapchain,
  SamplerAnisotropy,
};

enum class EFilter : uint8_t {
  Nearest,
  Linear,
};

enum class EMipmapMode : uint8_t {
  Nearest,
  Linear,
};

enum class EAddressMode : uint8_t {
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder,
  MirrorClampToEdge,
};

enum class EBlendOp : uint32_t {
  Add = 0,
  Subtract = 1,
  ReverseSubtract = 2,
  Min = 3,
  Max = 4
};

enum class EBlendFactor : uint32_t {
  Zero = 0,
  One = 1,
  SrcColor = 2,
  OneMinusSrcColor = 3,
  DstColor = 4,
  OneMinusDstColor = 5,
  SrcAlpha = 6,
  OneMinusSrcAlpha = 7,
  DstAlpha = 8,
  OneMinusDstAlpha = 9,
  ConstantColor = 10,
  OneMinusConstantColor = 11,
  SrcAlphaSaturate = 12,
  Src1Color = 15,
};

enum class EFrontFace : uint32_t { Clockwise, CounterClockwise };

enum class EStencilOp : uint32_t {
  Keep = 0,
  Zero = 1,
  Replace = 2,
  IncrementClamp = 3,
  DecrementClamp = 4,
  Invert = 5,
  IncrementWrap = 6,
  DecrementWrap = 7,
};

enum class EColorWriteMask : uint32_t {
  None = 0,
  R = 1 << 0,
  G = 1 << 1,
  B = 1 << 2,
  A = 1 << 3,
  All = R | G | B | A,
};

template <> struct EnableBitmaskOperators<EColorWriteMask> : std::true_type {};
template <> struct EnableBitmaskOperators<EBufferUsage> : std::true_type {};
template <> struct EnableBitmaskOperators<EShaderStage> : std::true_type {};
template <> struct EnableBitmaskOperators<EMemoryProperty> : std::true_type {};
template <> struct EnableBitmaskOperators<ETextureUsage> : std::true_type {};
template <>
struct EnableBitmaskOperators<EAttachmentIntent> : std::true_type {};

struct RenderTargetBinding {
  int32_t swapchainSlot = 0;
  Array<TextureHandle> externalAttachments;
};

struct DeviceCapabilities {
  struct Limits {
    size_t minUniformBufferOffsetAlignment;
    size_t minStorageBufferOffsetAlignment;
    float maxSamplerAnisotroy;
  } limits;
};

struct QueueRequirement {
  bool isRequireGraphics = false;
  bool isRequireCompute = false;
  bool isRequireTransfer = false;
  bool isRequirePresent = false;
};

struct DeviceRequirement {
  QueueRequirement queueRequirement;
  Array<ERenderCapability> requiredCapabilities;
};

struct AttachmentDescription {
  StringId nameHash;
  EAttachmentIntent intent;
  EFormat format;
  ESampleCount sampleCount = ESampleCount::SampleCount1x;
  EAttachmentLoadOp loadOp;
  EAttachmentStoreOp storeOp;
  EResourceLayout initialLayout;
  EResourceLayout finalLayout;

  bool isAutoResize;
  bool isSwapchain;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;

    packed |= (static_cast<uint64_t>(format) & 0xFFFFULL);

    packed |= (static_cast<uint64_t>(sampleCount) & 0x0FULL) << 16;

    packed |= (static_cast<uint64_t>(loadOp) & 0x07ULL) << 20;
    packed |= (static_cast<uint64_t>(storeOp) & 0x07ULL) << 23;
    packed |= (static_cast<uint64_t>(initialLayout) & 0x1FULL) << 26;
    packed |= (static_cast<uint64_t>(finalLayout) & 0x1FULL) << 31;

    if (isSwapchain)
      packed |= (1ULL << 36);

    if (isAutoResize)
      packed |= (1ULL << 37);

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const AttachmentDescription &rhs) const noexcept {
    return isSwapchain == rhs.isSwapchain && isAutoResize == rhs.isAutoResize &&
           format == rhs.format && sampleCount == rhs.sampleCount &&
           loadOp == rhs.loadOp && storeOp == rhs.storeOp &&
           initialLayout == rhs.initialLayout && finalLayout == rhs.finalLayout;
  }
};

struct RenderPassCreateInfo {
  StringId nameHash;
  Array<AttachmentDescription> attachments;
  int32_t depthAttachmentIndex = -1;
  uint32_t samples = 1;

  HashType GetHash() const noexcept {
    HashType h = Hash::kOffsetBasis;

    for (const auto &att : attachments) {
      h = Hash::Combine(h, att.GetHash());
    }
    uint64_t states = 0;
    states |= (static_cast<uint64_t>(samples) & 0xFF);
    states |= (static_cast<uint64_t>(depthAttachmentIndex) << 32);

    h = Hash::Combine(h, states);
    return h;
  }

  bool operator==(const RenderPassCreateInfo &other) const noexcept {
    if (samples != other.samples ||
        depthAttachmentIndex != other.depthAttachmentIndex) {
      return false;
    }
    return attachments == other.attachments;
  }
};

struct SamplerCreateInfo {
  EFilter magFilter = EFilter::Linear;
  EFilter minFilter = EFilter::Linear;
  EMipmapMode mipmapMode = EMipmapMode::Linear;

  EAddressMode addressModeU = EAddressMode::ClampToEdge;
  EAddressMode addressModeV = EAddressMode::ClampToEdge;
  EAddressMode addressModeW = EAddressMode::ClampToEdge;

  float mipLodBias = 0.0f;
  float maxAnisotropy = 1;
  bool anisotropyEnable = false;

  bool compareEnable = false;
  ECompareOp compareOp = ECompareOp::Less;

  float minLod = 0.0f;
  float maxLod = 1000.0f;

  HashType GetHash() const noexcept {
    uint64_t h = Hash::kOffsetBasis;

    uint64_t packed = 0;
    packed |= (static_cast<uint64_t>(magFilter) & 0x01);
    packed |= (static_cast<uint64_t>(minFilter) & 0x01) << 1;
    packed |= (static_cast<uint64_t>(mipmapMode) & 0x01) << 2;
    packed |= (static_cast<uint64_t>(addressModeU) & 0x07) << 3;
    packed |= (static_cast<uint64_t>(addressModeV) & 0x07) << 6;
    packed |= (static_cast<uint64_t>(addressModeW) & 0x07) << 9;
    packed |= (static_cast<uint64_t>(compareOp) & 0x07) << 12;
    packed |= (anisotropyEnable ? 1ULL : 0ULL) << 15;
    packed |= (compareEnable ? 1ULL : 0ULL) << 16;

    h = Hash::Combine(h, packed);

    auto f1 = std::bit_cast<uint32_t>(mipLodBias);
    auto f2 = std::bit_cast<uint32_t>(maxAnisotropy);
    h = Hash::Combine(h, (static_cast<uint64_t>(f1) << 32) | f2);

    auto f3 = std::bit_cast<uint32_t>(minLod);
    auto f4 = std::bit_cast<uint32_t>(maxLod);
    h = Hash::Combine(h, (static_cast<uint64_t>(f3) << 32) | f4);

    return h;
  }

  bool operator==(const SamplerCreateInfo &other) const noexcept {
    return std::memcmp(this, &other, sizeof(SamplerCreateInfo)) == 0;
  }
};

struct BufferCreateInfo {
  uint64_t size;
  EBufferUsage usage;
  EMemoryProperty memoryProperty;
};

struct TextureCreateInfo {
  uint32_t width = 1;
  uint32_t height = 1;
  uint32_t depth = 1;
  uint32_t layers = 1;
  uint32_t mipLevels = 1;

  EFormat format = EFormat::B8G8R8A8_SRGB;
  ETextureUsage usage = ETextureUsage::ColorAttachment | ETextureUsage::Sampled;
};

struct VertexBinding {
  uint32_t binding;
  uint32_t stride;
  bool isInstanceData = false;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;

    packed |= (static_cast<uint64_t>(binding) & 0xFFFFULL);
    packed |= (static_cast<uint64_t>(stride) & 0xFFFFULL) << 16;
    packed |= (isInstanceData ? 1ULL : 0ULL) << 32;

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const VertexBinding &other) const noexcept {
    return binding == other.binding && stride == other.stride &&
           isInstanceData == other.isInstanceData;
  }
};

struct VertexInputAttribute {
  uint32_t location;
  uint32_t binding;
  rhi::EFormat format;
  EVertexSemantic semantic;
  uint32_t offset;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;

    packed |= (static_cast<uint64_t>(location) & 0xFFFFULL);
    packed |= (static_cast<uint64_t>(binding) & 0x3FULL) << 16;
    packed |= (static_cast<uint64_t>(format) & 0x3FFULL) << 22;
    packed |= (static_cast<uint64_t>(semantic) & 0x1FULL) << 32;
    packed |= (static_cast<uint64_t>(offset) & 0xFFFFULL) << 37;

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const VertexInputAttribute &other) const noexcept {
    return location == other.location && binding == other.binding &&
           format == other.format && semantic == other.semantic &&
           offset == other.offset;
  }
};

struct ShaderStageInfo {
  EShaderStage stage;
  String entryName;
  SharedBlobPtr shaderCode;

  HashType GetHash() const noexcept {
    auto hash = Hash::Combine(Hash::kOffsetBasis, static_cast<HashType>(stage));
    hash = Hash::Combine(hash, StringView(entryName).GetHash());
    hash = Hash::Combine(hash, shaderCode->GetHash());
    return hash;
  }

  bool operator==(const ShaderStageInfo &other) const {
    if (stage != other.stage || entryName != other.entryName)
      return false;
    if (shaderCode->GetData() == other.shaderCode->GetData() &&
        shaderCode->GetSize() == other.shaderCode->GetSize())
      return true;

    return memcmp(shaderCode->GetData(), other.shaderCode->GetData(),
                  shaderCode->GetSize()) == 0;
  }
};

struct DescriptorSetLayoutBinding {
  StringId nameHash;
  uint32_t binding;
  uint32_t set;
  EDescriptorType type;
  EShaderStage visibleStages;
  uint32_t count;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;
    packed |= (static_cast<uint64_t>(binding) & 0x1FULL);
    packed |= (static_cast<uint64_t>(set) & 0x07ULL) << 5;
    packed |= (static_cast<uint64_t>(type) & 0x0FULL) << 8;
    packed |= (static_cast<uint64_t>(visibleStages) & 0x3FFULL) << 12;
    packed |= (static_cast<uint64_t>(count) & 0x3FFULL) << 22;

    HashType h = Hash::Combine(Hash::kOffsetBasis, nameHash.GetHash());
    return Hash::Combine(h, packed);
  }

  bool operator==(const DescriptorSetLayoutBinding &other) const noexcept {
    return nameHash == other.nameHash && binding == other.binding &&
           set == other.set && type == other.type &&
           visibleStages == other.visibleStages && count == other.count;
  }
};

struct PushConstantRange {
  EShaderStage visibleStages;
  uint32_t offset;
  uint32_t size;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;

    packed |= (static_cast<uint64_t>(visibleStages) & 0x0FFFULL);
    packed |= (static_cast<uint64_t>(offset) & 0x0FFFULL) << 12;
    packed |= (static_cast<uint64_t>(size) & 0x0FFFULL) << 24;

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const PushConstantRange &other) const noexcept {
    return visibleStages == other.visibleStages && offset == other.offset &&
           size == other.size;
  }
};

struct BufferWriteInfo {
  BufferHandle buffer;
  uint64_t offset = 0;
  uint64_t range = 0;

  static BufferWriteInfo Whole(BufferHandle h) { return {h, 0, 0}; }
};

struct InputAssemblyState {
  EPrimitiveTopology topology = EPrimitiveTopology::TriangleList;
  bool primitiveRestartEnable = false;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;
    packed |= (static_cast<uint64_t>(topology) & 0xFULL);
    packed |= (primitiveRestartEnable ? 1ULL : 0ULL) << 4;

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const InputAssemblyState &other) const noexcept {
    return topology == other.topology &&
           primitiveRestartEnable == other.primitiveRestartEnable;
  }
};

struct RasterizationState {
  EPolygonMode polygonMode = EPolygonMode::Fill;
  ECullMode cullMode = ECullMode::Back;
  EFrontFace frontFace = EFrontFace::CounterClockwise;
  float lineWidth = 1.0f;
  bool depthBiasEnable = false;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;
    packed |= (static_cast<uint64_t>(polygonMode) & 0x7ULL);
    packed |= (static_cast<uint64_t>(cullMode) & 0x7ULL) << 3;
    packed |= (static_cast<uint64_t>(frontFace) & 0x3ULL) << 6;
    packed |= (depthBiasEnable ? 1ULL : 0ULL) << 8;

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const RasterizationState &other) const noexcept {
    return polygonMode == other.polygonMode && cullMode == other.cullMode &&
           frontFace == other.frontFace && lineWidth == other.lineWidth &&
           depthBiasEnable == other.depthBiasEnable;
  }
};

struct DepthStencilState {
  bool isDepthTestEnable = true;
  bool isDepthWriteEnable = true;
  ECompareOp depthCompareOp = ECompareOp::Less;
  bool isStencilTestEnable = false;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;
    packed |= (isDepthTestEnable ? 1ULL : 0ULL);
    packed |= (isDepthWriteEnable ? 1ULL : 0ULL) << 1;
    packed |= (static_cast<uint64_t>(depthCompareOp) & 0xFULL) << 2;
    packed |= (isStencilTestEnable ? 1ULL : 0ULL) << 6;

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const DepthStencilState &other) const noexcept {
    return isDepthTestEnable == other.isDepthTestEnable &&
           isDepthWriteEnable == other.isDepthWriteEnable &&
           depthCompareOp == other.depthCompareOp &&
           isStencilTestEnable == other.isStencilTestEnable;
  }
};

struct MultisampleState {
  ESampleCount sampleCount = ESampleCount::SampleCount1x;
  bool sampleShadingEnable = false;
  bool alphaToCoverageEnable = false;
  bool alphaToOneEnable = false;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;
    packed |= (static_cast<uint64_t>(sampleCount) & 0x7FULL);
    packed |= (sampleShadingEnable ? 1ULL : 0ULL) << 7;
    packed |= (alphaToCoverageEnable ? 1ULL : 0ULL) << 8;
    packed |= (alphaToOneEnable ? 1ULL : 0ULL) << 9;

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const MultisampleState &other) const noexcept {
    return sampleCount == other.sampleCount &&
           sampleShadingEnable == other.sampleShadingEnable &&
           alphaToCoverageEnable == other.alphaToCoverageEnable &&
           alphaToOneEnable == other.alphaToOneEnable;
  }
};

struct ColorBlendState {
  bool isEnable = false;
  EBlendOp colorOp = EBlendOp::Add;
  EBlendFactor srcColorFactor = EBlendFactor::One;
  EBlendFactor dstColorFactor = EBlendFactor::Zero;
  EBlendOp alphaOp = EBlendOp::Add;
  EBlendFactor srcAlphaFactor = EBlendFactor::One;
  EBlendFactor dstAlphaFactor = EBlendFactor::Zero;
  EColorWriteMask writeMask = EColorWriteMask::All;

  HashType GetHash() const noexcept {
    uint64_t packed = 0;

    packed |= (isEnable ? 1ULL : 0ULL);                              // bit 0
    packed |= (static_cast<uint64_t>(colorOp) & 0x7ULL) << 1;        // bits 1-3
    packed |= (static_cast<uint64_t>(srcColorFactor) & 0xFULL) << 4; // bits 4-7
    packed |= (static_cast<uint64_t>(dstColorFactor) & 0xFULL)
              << 8; // bits 8-11

    packed |= (static_cast<uint64_t>(alphaOp) & 0x7ULL) << 12; // bits 12-14
    packed |= (static_cast<uint64_t>(srcAlphaFactor) & 0xFULL)
              << 15; // bits 15-18
    packed |= (static_cast<uint64_t>(dstAlphaFactor) & 0xFULL)
              << 19; // bits 19-22

    packed |= (static_cast<uint64_t>(writeMask) & 0xFULL) << 23; // bits 23-26

    return Hash::Combine(Hash::kOffsetBasis, packed);
  }

  bool operator==(const ColorBlendState &other) const noexcept {
    return isEnable == other.isEnable && colorOp == other.colorOp &&
           srcColorFactor == other.srcColorFactor &&
           dstColorFactor == other.dstColorFactor && alphaOp == other.alphaOp &&
           srcAlphaFactor == other.srcAlphaFactor &&
           dstAlphaFactor == other.dstAlphaFactor &&
           writeMask == other.writeMask;
  }
};

struct PipelineCreateInfo {
  RenderPassHandle renderPassHandle;
  Span<const PushConstantRange> pushConstantRanges;
  Span<const VertexInputAttribute> vertexInputAttributes;
  Span<const VertexBinding> vertexBindings;
  Span<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
  Span<const ShaderStageInfo> stageInfos;
  InputAssemblyState inputAssemblyState;
  RasterizationState rasterizationState;
  MultisampleState multisampleState;
  DepthStencilState depthStencilState;
  Span<const ColorBlendState> colorBlendStates;
};

struct RingAllocation {
  void *pHostAddress;
  uint32_t offset;
  BufferHandle buffer;
};

struct Offset2D {
  int32_t x;
  int32_t y;
};

struct Extent2D {
  uint32_t width;
  uint32_t height;
};

struct Rect2D {
  Offset2D offset;
  Extent2D extent;
};

struct Viewport {
  float x;
  float y;
  float width;
  float height;
  float minDepth = 0.0f;
  float maxDepth = 1.0f;
};

struct DepthStencil {
  float depth;
  uint32_t stencil;
};

struct ClearValue {
  struct ColorValue {
    float r, g, b, a;
  };
  union {
    ColorValue color;
    DepthStencil depthStencil;
  };
  bool isDepth = false;

  static ClearValue Color(float r, float g, float b, float a = 1.0f) {
    ClearValue v;
    v.color = {r, g, b, a};
    v.isDepth = false;
    return v;
  }

  static ClearValue DepthStencil(float d = 1.0f, uint32_t s = 0) {
    ClearValue v;
    v.depthStencil = {d, s};
    v.isDepth = true;
    return v;
  }
};

struct RenderPassBeginInfo {
  RenderPassHandle renderPassHandle;
  RenderTargetBinding targets;
  Rect2D renderArea;

  Array<ClearValue> clearValues;
};

struct BufferCopy {
  uint64_t srcOffset = 0;
  uint64_t dstOffset = 0;
  uint64_t size = 0;
};

enum class ERhiResult {
  Success,
  Unknown,
  InitializationFailed,
  SurfaceLost,
  DeviceLost,
  OutOfMemory,
  BackendSpecificError,
  SwapchainOutOfDate,
  FailedToRecordCommand,
  FailedToSubmitQueue,
  FormatNotSupported,
};

} // namespace avalon::rhi
