module;
#include <cstdint>
#include <cstring>
#include <type_traits>

export module avalon.rhi:types;

import avalon.core;

export namespace avalon::rhi {

using BufferHandle = Handle<class BufferTag>;
using TextureHandle = Handle<class TextureTag>;
using PipelineHandle = Handle<class PipelineTag>;
using RenderPassHandle = Handle<class RenderPassTag>;
using FrameBufferHandle = Handle<class FramebufferTag>;

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
enum class EDepthCompareOp : uint32_t {
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
enum class EMemoryProperty : uint8_t {
  None = 0,
  DeviceLocal = 1 << 0,
  HostVisible = 1 << 1,
  HostCoherent = 1 << 2,
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

enum class ETextureUsage : uint32_t {
  None = 0,
  Sampled = 1 << 0,
  ColorAttachment = 1 << 1,
  DepthStencilAttachment = 1 << 2,
  Storage = 1 << 3,
  TransferSrc = 1 << 4,
};

enum class EQueueType { Graphics, Transfer, Compute, Present };

enum class ERenderTarget : uint32_t {
  SwapchainBackBuffer = 0,
};

enum class ERenderCapability : uint32_t {
  Swapchain,
  SamplerAnisotropy,
};

template <> struct EnableBitmaskOperators<EBufferUsage> : std::true_type {};
template <> struct EnableBitmaskOperators<EShaderStage> : std::true_type {};
template <> struct EnableBitmaskOperators<EMemoryProperty> : std::true_type {};

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
  EFormat format;
  EAttachmentLoadOp loadOp;
  EAttachmentStoreOp storeOp;
  EResourceLayout initialLayout;
  EResourceLayout finalLayout;

  HashType GetHash() const noexcept {
    auto hash = Hash::kOffsetBasis;
    hash = Hash::Combine(hash, static_cast<HashType>(format));
    hash = Hash::Combine(hash, static_cast<HashType>(loadOp));
    hash = Hash::Combine(hash, static_cast<HashType>(storeOp));
    hash = Hash::Combine(hash, static_cast<HashType>(initialLayout));
    hash = Hash::Combine(hash, static_cast<HashType>(finalLayout));
    return hash;
  }

  bool operator==(const AttachmentDescription &other) const {
    if (format != other.format || loadOp != other.loadOp ||
        storeOp != other.storeOp || initialLayout != other.initialLayout ||
        finalLayout != other.finalLayout)
      return false;
    return true;
  }
};

struct RenderPassCreateInfo {
  Array<AttachmentDescription> colorAttachments;
  AttachmentDescription depthAttachment;
  bool hasDepth = false;
  uint32_t samples = 1;

  HashType GetHash() const noexcept {
    auto hash = Hash::kOffsetBasis;
    for (const auto &colorAttachment : colorAttachments) {
      hash = Hash::Combine(hash, colorAttachment.GetHash());
    }
    hash = Hash::Combine(hash, static_cast<uint64_t>(hasDepth));
    if (hasDepth) {
      hash = Hash::Combine(hash, depthAttachment.GetHash());
    }
    hash = Hash::Combine(hash, static_cast<uint64_t>(samples));
    return hash;
  }

  bool operator==(const RenderPassCreateInfo &other) const {
    if (hasDepth != other.hasDepth || samples != other.samples)
      return false;
    if (colorAttachments.GetSize() != other.colorAttachments.GetSize())
      return false;

    for (uint32_t i = 0; i < colorAttachments.GetSize(); ++i) {
      if (!(colorAttachments[i] == other.colorAttachments[i]))
        return false;
    }
    if (hasDepth && !(depthAttachment == other.depthAttachment))
      return false;
    return true;
  }
};

struct FrameBufferCreateInfo {
  RenderPassHandle renderPassHandle;
  Array<TextureHandle> attachments;
  uint32_t width;
  uint32_t height;
  uint32_t layers = 1;
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

  EFormat format = EFormat::R16G16B16A16_Uint;
};

struct VertexBinding {
  uint32_t binding;
  uint32_t stride;
  bool isInstanceData = false;

  HashType GetHash() const noexcept {
    auto hash =
        Hash::Combine(Hash::kOffsetBasis, static_cast<HashType>(binding));
    hash = Hash::Combine(hash, static_cast<HashType>(stride));
    hash = Hash::Combine(hash, static_cast<HashType>(isInstanceData));
    return hash;
  }

  bool operator==(const VertexBinding &other) const {
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
    auto hash =
        Hash::Combine(Hash::kOffsetBasis, static_cast<HashType>(location));
    hash = Hash::Combine(hash, static_cast<HashType>(binding));
    hash = Hash::Combine(hash, static_cast<HashType>(format));
    hash = hash = Hash::Combine(hash, static_cast<HashType>(offset));
    return hash;
  }

  bool operator==(const VertexInputAttribute &other) const {
    return location == other.location && binding == other.binding &&
           format == other.format && offset == other.offset;
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
  uint32_t binding;
  uint32_t set;
  EDescriptorType type;
  EShaderStage visibleStages;
  uint32_t count;

  HashType GetHash() const noexcept {
    auto hash =
        Hash::Combine(Hash::kOffsetBasis, static_cast<HashType>(binding));
    hash = Hash::Combine(hash, static_cast<HashType>(set));
    hash = Hash::Combine(hash, static_cast<HashType>(type));
    hash = Hash::Combine(hash, static_cast<HashType>(visibleStages));
    hash = Hash::Combine(hash, static_cast<HashType>(count));
    return hash;
  }

  bool operator==(const DescriptorSetLayoutBinding &other) const {
    return binding == other.binding && set == other.set && type == other.type &&
           visibleStages == other.visibleStages && count == other.count;
  }
};

struct PushConstantRange {
  EShaderStage visibleStages;
  uint32_t offset;
  uint32_t size;

  HashType GetHash() const noexcept {
    auto hash =
        Hash::Combine(Hash::kOffsetBasis, static_cast<HashType>(visibleStages));
    hash = Hash::Combine(hash, static_cast<HashType>(offset));
    hash = Hash::Combine(hash, static_cast<HashType>(size));
    return hash;
  }

  bool operator==(const PushConstantRange &other) const {
    return visibleStages == other.visibleStages && offset == other.offset &&
           size == other.size;
  }
};

struct PipelineCreateInfo {
  RenderPassHandle renderPassHandle;
  Span<const PushConstantRange> pushConstantRanges;
  Span<const VertexInputAttribute> vertexInputAttributes;
  Span<const VertexBinding> vertexBindings;
  Span<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
  Span<const ShaderStageInfo> stageInfos;
  EPrimitiveTopology topology = EPrimitiveTopology::TriangleList;
  EPolygonMode polygonMode = EPolygonMode::Fill;
  ECullMode cullMode = ECullMode::None;
  bool isDepthTestEnable = false;
  bool isDepthWriteEnable = false;
  EDepthCompareOp depthCompareOp = EDepthCompareOp::Less;

  float lineWidth = 1.0f;
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

struct Color {
  float r, g, b, a;
};

struct DepthStencil {
  float depth;
  uint32_t stencil;
};

struct ClearValue {
  union {
    Color color;
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
  ERenderTarget renderTarget;
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
