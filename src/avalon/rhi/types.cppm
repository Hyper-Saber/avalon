module;
#include <bit>
#include <cstdint>
#include <cstring>
#include <optional>
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

constexpr TextureHandle kSwapchainColorHandle = TextureHandle::Internal();

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
  R16_UNORM,
  R16G16_UNORM,
  R16G16B16A16_UNORM,

  R8G8B8_SRGB,
  R8G8B8A8_SRGB,
  B8G8R8A8_SRGB,

  R16_SFLOAT,
  R16G16_SFLOAT,
  R16G16B16A16_SFLOAT,

  D32_SFLOAT,
  D32_SFLOAT_S8_UINT,
};

enum class EPipelineBindPoint {
  Graphics,
  Compute,
  RayTrace,
};

enum class EPassType {
  Graphics,
  Compute,
};

enum class EVertexSemantic {
  Unknown,
  Position,
  TexCoord,
  Color,
  Normal,
};

enum class EShaderStage : uint32_t {
  None = 0,

  Vertex = 1 << 0,
  TessControl = 1 << 1,
  TessEvaluation = 1 << 2,
  Geometry = 1 << 3,
  Fragment = 1 << 4,

  Compute = 1 << 5,

  Task = 1 << 6,
  Mesh = 1 << 7,

  RayGen = 1 << 8,
  RayAnyHit = 1 << 9,
  RayClosestHit = 1 << 10,
  RayMiss = 1 << 11,
  RayIntersection = 1 << 12,
  Callable = 1 << 13,

  AllGraphics = Vertex | TessControl | TessEvaluation | Geometry | Fragment,

  AllMesh = Task | Mesh | Fragment,

  AllRayTracing =
      RayGen | RayAnyHit | RayClosestHit | RayMiss | RayIntersection | Callable,

  All = 0x7FFFFFFF
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
  NotEqual,
  Always,
};

enum class EResourceLayout {
  Undefined,
  ColorAttachment,
  DepthStencilAttachment,
  DepthStencilReadOnly,
  ShaderReadOnly,
  Present,
  TransferSrc,
  TransferDst,
  General,
};

enum class EResourceUsage : uint32_t {
  None = 0,
  VertexBuffer = 1 << 0,
  IndexBuffer = 1 << 1,
  IndirectBuffer = 1 << 2,
  UniformBuffer = 1 << 3,

  StorageBuffer = 1 << 4,

  ReadOnly = 1 << 5,
  ReadWrite = 1 << 6,

  ColorAttachment = 1 << 7,
  DepthStencilAttachment = 1 << 8,

  TransferSrc = 1 << 9,
  TransferDst = 1 << 10,

  Present = 1 << 11,

  SceneGlobals = 1 << 12,

  Host = 1 << 13,
  DepthTexture = 1 << 14,
  StencilTexture = 1 << 15,
};

enum class ETextureType {
  Texture2D,
  TextureCube,
  Texture2DArray,
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

enum class ESampleCount {
  SampleCount1x,
  SampleCount2x,
  SampleCount4x,
  SampleCount8x,
  SampleCount16x,
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

enum class EAccess : uint32_t {
  None = 0,
  ColorRead = 1 << 0,
  ColorWrite = 1 << 1,
  DepthStencilRead = 1 << 2,
  DepthStencilWrite = 1 << 3,
  TextureRead = 1 << 4,
  TransferRead = 1 << 5,
  TransferWrite = 1 << 6,
  MemoryRead = 1 << 7,
  MemoryWrite = 1 << 8,

  IndirectCommandRead = 1 << 9,
  IndexRead = 1 << 10,
  VertexAttributeRead = 1 << 11,
  UniformRead = 1 << 12,
  StorageRead = 1 << 13,
  StorageWrite = 1 << 14,

  HostWrite = 1 << 15,
  HostRead = 1 << 16,
};

enum class EPipelineStage : uint64_t {
  None = 0,

  TopOfPipe = 1ULL << 0,
  BottomOfPipe = 1ULL << 1,

  DrawIndirect = 1ULL << 2,
  VertexInput = 1ULL << 3,

  VertexShader = 1ULL << 4,
  TessControlShader = 1ULL << 5,
  TessEvaluationShader = 1ULL << 6,
  GeometryShader = 1ULL << 7,
  FragmentShader = 1ULL << 8,
  ComputeShader = 1ULL << 9,

  EarlyFragmentTests = 1ULL << 10,
  LateFragmentTests = 1ULL << 11,
  ColorAttachmentOutput = 1ULL << 12,

  Transfer = 1ULL << 13,
  Clear = 1ULL << 14,

  Host = 1ULL << 15,

  AllGraphics = 1ULL << 16,
  AllCommands = 1ULL << 17,

  RayTracingShader = 1ULL << 18,
};

template <> struct EnableBitmaskOperators<EColorWriteMask> : std::true_type {};
template <> struct EnableBitmaskOperators<EResourceUsage> : std::true_type {};
template <> struct EnableBitmaskOperators<EShaderStage> : std::true_type {};
template <> struct EnableBitmaskOperators<EMemoryProperty> : std::true_type {};
template <>
struct EnableBitmaskOperators<EAttachmentIntent> : std::true_type {};
template <> struct EnableBitmaskOperators<EAccess> : std::true_type {};
template <> struct EnableBitmaskOperators<EPipelineStage> : std::true_type {};

//---------------------------------------------------------------------------------------------------------------------

constexpr uint32_t kMaxCustomSlots = 16;
constexpr uint32_t kPushConstantFloatSize = 32 + 1 + kMaxCustomSlots;
constexpr uint32_t kInvalidTextureSlot = 0xFFFF;

constexpr uint32_t kSkyboxSHSlot = 0;
constexpr uint32_t kSkyboxPrefilteredSlot = 1;
constexpr uint32_t kBRDFLutSlot = 2;

struct IndexedIndirectCommand {
  uint32_t indexCount;
  uint32_t instanceCount;
  uint32_t firstIndex;
  int32_t vertexOffset;
  uint32_t firstInstance;
};

struct alignas(16) StandardPushConstants {
  uint32_t instanceBufferOffset;
  uint32_t skyboxSHOffset;
  uint32_t prefilterMap;
  uint32_t brdfLut;

  uint32_t shadowMask;
  uint32_t paddings[kPushConstantFloatSize - 5];
};

struct DeviceCapabilities {
  struct Limits {
    size_t minUniformBufferOffsetAlignment;
    size_t minStorageBufferOffsetAlignment;
    size_t maxHostLocalVisibleMemorySize;
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
  EResourceUsage usage;
  EMemoryProperty memoryProperty;
};

struct TextureCreateInfo {
  StringId nameHash;
  uint32_t width = 1;
  uint32_t height = 1;
  uint32_t depth = 1;
  uint32_t layerCount = 1;
  uint32_t mipLevels = 1;

  EFormat format = EFormat::B8G8R8A8_SRGB;
  EResourceUsage usage =
      EResourceUsage::ColorAttachment | EResourceUsage::ReadOnly;
  ESampleCount sampleCount = ESampleCount::SampleCount1x;
  ETextureType textureType = ETextureType::Texture2D;
};

struct ComputeGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
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

  String ToString() const;
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

  String ToString() const;
};

struct StencilFaceState {
  EStencilOp failOp = EStencilOp::Keep;
  EStencilOp passOp = EStencilOp::Keep;
  EStencilOp depthFailOp = EStencilOp::Keep;
  ECompareOp compareOp = ECompareOp::Always;
  uint32_t compareMask = 0xFF;
  uint32_t writeMask = 0xFF;
  uint32_t reference = 0;

  uint64_t Pack() const noexcept {
    uint64_t p = 0;
    p |= (static_cast<uint64_t>(failOp) & 0x7);
    p |= (static_cast<uint64_t>(passOp) & 0x7) << 3;
    p |= (static_cast<uint64_t>(depthFailOp) & 0x7) << 6;
    p |= (static_cast<uint64_t>(compareOp) & 0xF) << 9;
    return p;
  }
};

struct DepthStencilState {
  bool isDepthTestEnable = true;
  bool isDepthWriteEnable = true;
  ECompareOp depthCompareOp = ECompareOp::Greater;

  bool isStencilTestEnable = false;
  StencilFaceState front{};
  StencilFaceState back{};

  HashType GetHash() const noexcept {
    uint64_t packed = 0;
    packed |= (isDepthTestEnable ? 1ULL : 0ULL);
    packed |= (isDepthWriteEnable ? 1ULL : 0ULL) << 1;
    packed |= (static_cast<uint64_t>(depthCompareOp) & 0xFULL) << 2;
    packed |= (isStencilTestEnable ? 1ULL : 0ULL) << 6;

    HashType h = Hash::Combine(Hash::kOffsetBasis, packed);
    if (isStencilTestEnable) {
      h = Hash::Combine(h, front.Pack());
      h = Hash::Combine(h, front.compareMask);
      h = Hash::Combine(h, front.writeMask);
      h = Hash::Combine(h, front.reference);

      h = Hash::Combine(h, back.Pack());
      h = Hash::Combine(h, back.compareMask);
      h = Hash::Combine(h, back.writeMask);
      h = Hash::Combine(h, back.reference);
    }
    return h;
  }

  bool operator==(const DepthStencilState &other) const noexcept {
    bool basic = isDepthTestEnable == other.isDepthTestEnable &&
                 isDepthWriteEnable == other.isDepthWriteEnable &&
                 depthCompareOp == other.depthCompareOp &&
                 isStencilTestEnable == other.isStencilTestEnable;

    if (!basic || !isStencilTestEnable)
      return basic;

    return memcmp(&front, &other.front, sizeof(StencilFaceState)) == 0 &&
           memcmp(&back, &other.back, sizeof(StencilFaceState)) == 0;
  }

  String ToString() const;
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
  EBlendFactor srcColorFactor = EBlendFactor::SrcAlpha;
  EBlendFactor dstColorFactor = EBlendFactor::OneMinusSrcAlpha;
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

struct PipelineRenderingInfo {
  Array<EFormat> colorAttachmentFormats;
  EFormat depthAttachmentFormat = EFormat::Undefined;
  EFormat stencilAttachmentFormat = EFormat::Undefined;
  uint32_t viewMask = 0;

  void Clear() { *this = PipelineRenderingInfo{}; }

  HashType GetHash() const noexcept {
    uint64_t packed = 0;
    packed |=
        (static_cast<uint64_t>(depthAttachmentFormat) & 0x3FFULL); // bits 0-9
    packed |= (static_cast<uint64_t>(stencilAttachmentFormat) & 0x3FFULL)
              << 10; // bits 10-19
    packed |= (static_cast<uint64_t>(viewMask) & 0xFFFFFFULL)
              << 20; // bits 20-43

    HashType hash = Hash::Combine(Hash::kOffsetBasis, packed);

    for (const auto &format : colorAttachmentFormats) {
      hash = Hash::Combine(hash, static_cast<uint64_t>(format));
    }

    return hash;
  }

  bool operator==(const PipelineRenderingInfo &other) const noexcept {
    return viewMask == other.viewMask &&
           depthAttachmentFormat == other.depthAttachmentFormat &&
           stencilAttachmentFormat == other.stencilAttachmentFormat &&
           colorAttachmentFormats == other.colorAttachmentFormats;
  }
};

struct ComputePipelineCreateInfo {
  const ShaderStageInfo &stageInfo;
  Span<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
};

struct PipelineCreateInfo {
  PipelineRenderingInfo renderingInfo;

  Span<const VertexInputAttribute> vertexInputAttributes;
  Span<const VertexBinding> vertexBindings;

  Span<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings;

  Span<const ShaderStageInfo> stageInfos;
  InputAssemblyState inputAssemblyState;
  RasterizationState rasterizationState;
  MultisampleState multisampleState;
  DepthStencilState depthStencilState;
  Span<const ColorBlendState> colorBlendStates;

  HashType GetHash() const noexcept;
};

struct BufferAllocation {
  void *pHostAddress;
  BufferHandle buffer;
  uint32_t offset;
  uint32_t size;
};

struct Offset2D {
  int32_t x = 0;
  int32_t y = 0;
  auto operator<=>(const Offset2D &) const = default;
};

struct Extent2D {
  uint32_t width = 0;
  uint32_t height = 0;

  Extent2D operator*(float a) {
    return {static_cast<uint32_t>(a * width),
            static_cast<uint32_t>(a * height)};
  }

  bool IsValid() { return width != 0 && height != 0; }

  auto operator<=>(const Extent2D &) const = default;
};

struct Rect2D {
  Offset2D offset{};
  Extent2D extent{};

  auto operator<=>(const Rect2D &) const = default;

  bool IsValid() {
    return extent.width - offset.x > 0 && extent.height - offset.y > 0;
  }
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
  float depth = 0;
  uint32_t stencil = 0;
};

struct ClearValue {
  union {
    Color color{};
    DepthStencil depthStencil;
  };
  bool isDepth = false;

  ClearValue() : color{0, 0, 0, 0}, isDepth(false) {}

  static ClearValue Black() {
    ClearValue v;
    v.color = {0.0f, 0.0f, 0.0f, 1.0f};
    v.isDepth = false;
    return v;
  }

  static ClearValue FromColor(Color color) {
    ClearValue v;
    v.color = {color.r, color.g, color.b, color.a};
    v.isDepth = false;
    return v;
  }

  static ClearValue Color(float r, float g, float b, float a = 1.0f) {
    ClearValue v;
    v.color = {r, g, b, a};
    v.isDepth = false;
    return v;
  }

  static ClearValue DepthStencil(float d = 0.0f, uint32_t s = 0) {
    ClearValue v;
    v.depthStencil = {d, s};
    v.isDepth = true;
    return v;
  }
};

struct ColorAttachmentInfo {
  TextureHandle texture;
  EAttachmentLoadOp loadOp = EAttachmentLoadOp::Clear;
  EAttachmentStoreOp storeOp = EAttachmentStoreOp::Store;
  Color clearColor{0.0f, 0.0f, 0.0f, 1.0f};
  EResourceLayout layout = EResourceLayout::ColorAttachment;
  uint32_t layerCount = 1;
  uint32_t viewMask = 0;
};

struct DepthStencilAttachmentInfo {
  TextureHandle texture;
  EAttachmentLoadOp loadOp = EAttachmentLoadOp::Clear;
  EAttachmentStoreOp storeOp = EAttachmentStoreOp::Store;
  float clearDepth = 1.0f;
  uint32_t clearStencil = 0;
  EResourceLayout layout = EResourceLayout::DepthStencilAttachment;
  uint32_t layerCount = 1;
  uint32_t viewMask = 0;
};

struct RenderingInfo {
  Rect2D renderArea;
  uint32_t layerCount{1};
  uint32_t viewMask;
  Array<ColorAttachmentInfo> colorAttachments;
  std::optional<DepthStencilAttachmentInfo> depthStencil;
};

struct BufferCopyRegion {
  uint64_t srcOffset = 0;
  uint64_t dstOffset = 0;
  uint64_t size = 0;
};

struct ImageBarrier {
  TextureHandle texture;
  EResourceLayout oldLayout;
  EResourceLayout newLayout;
  EAccess srcAccess;
  EAccess dstAccess;
  EPipelineStage srcStage;
  EPipelineStage dstStage;

  uint32_t baseMipLevel = 0;
  uint32_t levelCount = 1;
  uint32_t baseArrayLayer = 0;
  uint32_t layerCount = 1;
};

struct BufferBarrier {
  BufferHandle buffer;
  EAccess srcAccess;
  EAccess dstAccess;
  EPipelineStage srcStage;
  EPipelineStage dstStage;

  uint32_t offset = 0;
  uint32_t size = 0;
};

struct ImageCopyRegion {
  Extent2D extent;
  Offset2D srcOffset{0, 0};
  Offset2D dstOffset{0, 0};
  uint32_t srcMipLevel = 0;
  uint32_t dstMipLevel = 0;
  uint32_t srcLayer = 0;
  uint32_t dstLayer = 0;
  uint32_t layerCount = 1;
};

struct ImageBlitRegion {
  Offset2D srcOffset{0, 0};
  Offset2D dstOffset{0, 0};
  uint32_t srcMipLevel = 0;
  uint32_t dstMipLevel = 0;
  uint32_t srcLayer = 0;
  uint32_t dstLayer = 0;
  uint32_t layerCount = 1;
};

struct StaticSamplers {
  uint32_t linearClamp;
  uint32_t pointClamp;
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
