module;
#include <debug/assert.hpp>
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:utils;

import avalon.rhi;
import avalon.core;
import :types;

namespace avalon::rhi {

auto TranslateRequirements(const DeviceRequirement &requirements)
    -> DeviceConfig {
  DeviceConfig config{.queueRequirement = requirements.queueRequirement};
  for (auto capability : requirements.requiredCapabilities) {
    switch (capability) {
    case ERenderCapability::Swapchain:
      config.extensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
      break;
    case ERenderCapability::SamplerAnisotropy:
      config.features.samplerAnisotropy = VK_TRUE;
      break;
    }
  }

  config.extensions.PushBack(VK_GOOGLE_HLSL_FUNCTIONALITY1_EXTENSION_NAME);

  return config;
}

constexpr auto ToVkFormat(EFormat format) noexcept -> VkFormat {
  switch (format) {
  case EFormat::Undefined:
    return VkFormat::VK_FORMAT_UNDEFINED;
  case EFormat::R16_Uint:
    return VkFormat::VK_FORMAT_R16_UINT;
  case EFormat::R16_Int:
    return VkFormat::VK_FORMAT_R16_SINT;
  case EFormat::R16_Float:
    return VkFormat::VK_FORMAT_R16_SFLOAT;
  case EFormat::R16G16_Uint:
    return VkFormat::VK_FORMAT_R16G16_UINT;
  case EFormat::R16G16_Int:
    return VkFormat::VK_FORMAT_R16G16_SINT;
  case EFormat::R16G16_Float:
    return VkFormat::VK_FORMAT_R16G16_SFLOAT;
  case EFormat::R16G16B16_Uint:
    return VkFormat::VK_FORMAT_R16G16B16_UINT;
  case EFormat::R16G16B16_Int:
    return VkFormat::VK_FORMAT_R16G16B16_SINT;
  case EFormat::R16G16B16_Float:
    return VkFormat::VK_FORMAT_R16G16B16_SFLOAT;
  case EFormat::R16G16B16A16_Uint:
    return VkFormat::VK_FORMAT_R16G16B16A16_UINT;
  case EFormat::R16G16B16A16_Int:
    return VkFormat::VK_FORMAT_R16G16B16A16_SINT;
  case EFormat::R16G16B16A16_Float:
    return VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
  case EFormat::R32_Uint:
    return VkFormat::VK_FORMAT_R32_UINT;
  case EFormat::R32_Int:
    return VkFormat::VK_FORMAT_R32_SINT;
  case EFormat::R32_Float:
    return VkFormat::VK_FORMAT_R32_SFLOAT;
  case EFormat::R32G32_Uint:
    return VkFormat::VK_FORMAT_R32G32_UINT;
  case EFormat::R32G32_Int:
    return VkFormat::VK_FORMAT_R32G32_SINT;
  case EFormat::R32G32_Float:
    return VkFormat::VK_FORMAT_R32G32_SFLOAT;
  case EFormat::R32G32B32_Uint:
    return VkFormat::VK_FORMAT_R32G32B32_UINT;
  case EFormat::R32G32B32_Int:
    return VkFormat::VK_FORMAT_R32G32B32_SINT;
  case EFormat::R32G32B32_Float:
    return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
  case EFormat::R32G32B32A32_Uint:
    return VkFormat::VK_FORMAT_R32G32B32A32_UINT;
  case EFormat::R32G32B32A32_Int:
    return VkFormat::VK_FORMAT_R32G32B32A32_SINT;
  case EFormat::R32G32B32A32_Float:
    return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
  case EFormat::R64_Uint:
    return VkFormat::VK_FORMAT_R64_UINT;
  case EFormat::R64_Int:
    return VkFormat::VK_FORMAT_R64_SINT;
  case EFormat::R64_Float:
    return VkFormat::VK_FORMAT_R64_SFLOAT;
  case EFormat::R64G64_Uint:
    return VkFormat::VK_FORMAT_R64G64_UINT;
  case EFormat::R64G64_Int:
    return VkFormat::VK_FORMAT_R64G64_SINT;
  case EFormat::R64G64_Float:
    return VkFormat::VK_FORMAT_R64G64_SFLOAT;
  case EFormat::R64G64B64_Uint:
    return VkFormat::VK_FORMAT_R64G64B64_UINT;
  case EFormat::R64G64B64_Int:
    return VkFormat::VK_FORMAT_R64G64B64_SINT;
  case EFormat::R64G64B64_Float:
    return VkFormat::VK_FORMAT_R64G64B64_SFLOAT;
  case EFormat::R64G64B64A64_Uint:
    return VkFormat::VK_FORMAT_R64G64B64A64_UINT;
  case EFormat::R64G64B64A64_Int:
    return VkFormat::VK_FORMAT_R64G64B64A64_SINT;
  case EFormat::R64G64B64A64_Float:
    return VkFormat::VK_FORMAT_R64G64B64A64_SFLOAT;
  case EFormat::R8G8B8_UNORM:
    return VkFormat::VK_FORMAT_R8G8B8_UNORM;
  case EFormat::R8G8B8A8_UNORM:
    return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
  case EFormat::R8G8B8_SRGB:
    return VkFormat::VK_FORMAT_R8G8B8_SRGB;
  case EFormat::R8G8B8A8_SRGB:
    return VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
  case EFormat::B8G8R8A8_SRGB:
    return VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
  case EFormat::R16G16B16A16_SFLOAT:
    return VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
  case EFormat::D32_SFLOAT:
    return VkFormat::VK_FORMAT_D32_SFLOAT;
  case EFormat::D32_SFLOAT_S8_UINT:
    return VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
  }
}

constexpr auto ToVkLoadOp(EAttachmentLoadOp loadOp) noexcept
    -> VkAttachmentLoadOp {
  switch (loadOp) {
  case EAttachmentLoadOp::Load:
    return VK_ATTACHMENT_LOAD_OP_LOAD;
  case EAttachmentLoadOp::Clear:
    return VK_ATTACHMENT_LOAD_OP_CLEAR;
  case EAttachmentLoadOp::DontCare:
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  }
}

constexpr auto ToVkStoreOp(EAttachmentStoreOp storeOp) noexcept
    -> VkAttachmentStoreOp {
  switch (storeOp) {
  case EAttachmentStoreOp::Store:
    return VK_ATTACHMENT_STORE_OP_STORE;
  case EAttachmentStoreOp::DontCare:
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }
}

constexpr auto ToVkBlendOp(EBlendOp op) noexcept -> VkBlendOp {
  switch (op) {
  case EBlendOp::Add:
    return VK_BLEND_OP_ADD;
  case EBlendOp::Subtract:
    return VK_BLEND_OP_SUBTRACT;
  case EBlendOp::ReverseSubtract:
    return VK_BLEND_OP_REVERSE_SUBTRACT;
  case EBlendOp::Min:
    return VK_BLEND_OP_MIN;
  case EBlendOp::Max:
    return VK_BLEND_OP_MAX;
  default:
    AVALON_ASSERT_MSG(false, "Unsupported BlendOp");
    return VK_BLEND_OP_ADD;
  }
}

constexpr auto ToVkBlendFactor(EBlendFactor factor) noexcept -> VkBlendFactor {
  switch (factor) {
  case EBlendFactor::Zero:
    return VK_BLEND_FACTOR_ZERO;
  case EBlendFactor::One:
    return VK_BLEND_FACTOR_ONE;
  case EBlendFactor::SrcColor:
    return VK_BLEND_FACTOR_SRC_COLOR;
  case EBlendFactor::OneMinusSrcColor:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  case EBlendFactor::DstColor:
    return VK_BLEND_FACTOR_DST_COLOR;
  case EBlendFactor::OneMinusDstColor:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
  case EBlendFactor::SrcAlpha:
    return VK_BLEND_FACTOR_SRC_ALPHA;
  case EBlendFactor::OneMinusSrcAlpha:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  case EBlendFactor::DstAlpha:
    return VK_BLEND_FACTOR_DST_ALPHA;
  case EBlendFactor::OneMinusDstAlpha:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  case EBlendFactor::ConstantColor:
    return VK_BLEND_FACTOR_CONSTANT_COLOR;
  case EBlendFactor::OneMinusConstantColor:
    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
  case EBlendFactor::SrcAlphaSaturate:
    return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
  case EBlendFactor::Src1Color:
    return VK_BLEND_FACTOR_SRC1_COLOR;
  default:
    AVALON_ASSERT_MSG(false, "Unsupported BlendFactor");
    return VK_BLEND_FACTOR_ONE;
  }
}

constexpr auto ToVkImageLayout(EResourceLayout layout) noexcept
    -> VkImageLayout {
  switch (layout) {
  case EResourceLayout::Undefined:
    return VK_IMAGE_LAYOUT_UNDEFINED;
  case EResourceLayout::ColorAttachment:
    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  case EResourceLayout::DepthStencilAttachment:
    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  case EResourceLayout::ShaderReadOnly:
    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  case EResourceLayout::Present:
    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  case EResourceLayout::TransferSrc:
    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  case EResourceLayout::TransferDst:
    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  }
}

constexpr auto ToVkPrimitiveTopology(EPrimitiveTopology topology) noexcept
    -> VkPrimitiveTopology {
  switch (topology) {
  case EPrimitiveTopology::PointList:
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  case EPrimitiveTopology::LineList:
    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  case EPrimitiveTopology::TriangleList:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  }
}

constexpr auto ToVkPolygonMode(EPolygonMode polygonMode) noexcept
    -> VkPolygonMode {
  switch (polygonMode) {
  case EPolygonMode::Fill:
    return VK_POLYGON_MODE_FILL;
  case EPolygonMode::Line:
    return VK_POLYGON_MODE_LINE;
  case EPolygonMode::Point:
    return VK_POLYGON_MODE_POINT;
    break;
  }
}

constexpr auto ToVkSampleCount(ESampleCount e) noexcept
    -> VkSampleCountFlagBits {
  switch (e) {
  case ESampleCount::SampleCount1x:
    return VK_SAMPLE_COUNT_1_BIT;
  case ESampleCount::SampleCount2x:
    return VK_SAMPLE_COUNT_2_BIT;
  case ESampleCount::SampleCount4x:
    return VK_SAMPLE_COUNT_4_BIT;
  case ESampleCount::SampleCount8x:
    return VK_SAMPLE_COUNT_8_BIT;
  case ESampleCount::SampleCount16x:
    return VK_SAMPLE_COUNT_16_BIT;
    break;
  }
}

constexpr auto ToVkCompareOp(ECompareOp compareOp) noexcept -> VkCompareOp {
  switch (compareOp) {
  case ECompareOp::Less:
    return VK_COMPARE_OP_LESS;
  case ECompareOp::LessOrEqual:
    return VK_COMPARE_OP_LESS_OR_EQUAL;
  case ECompareOp::Greater:
    return VK_COMPARE_OP_GREATER;
  case ECompareOp::GreaterOrEqual:
    return VK_COMPARE_OP_GREATER_OR_EQUAL;
  case ECompareOp::Equal:
    return VK_COMPARE_OP_EQUAL;
  case ECompareOp::NotEqual:
    return VK_COMPARE_OP_NOT_EQUAL;
  }
}

constexpr auto ToVkFilter(EFilter e) noexcept {
  switch (e) {
  case EFilter::Nearest:
    return VK_FILTER_NEAREST;
  case EFilter::Linear:
    return VK_FILTER_LINEAR;
  }
}

constexpr auto ToVkMipmapMode(EMipmapMode e) noexcept {
  switch (e) {
  case EMipmapMode::Nearest:
    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
  case EMipmapMode::Linear:
    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    break;
  }
}

constexpr auto ToVkAddressMode(EAddressMode e) noexcept {
  switch (e) {
  case EAddressMode::Repeat:
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  case EAddressMode::MirroredRepeat:
    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  case EAddressMode::ClampToEdge:
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  case EAddressMode::ClampToBorder:
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  case EAddressMode::MirrorClampToEdge:
    return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
  }
}

constexpr auto ToVkBufferUsageBits(EBufferUsage usage) noexcept
    -> VkBufferUsageFlagBits {
  switch (usage) {
  case EBufferUsage::Vertex:
    return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  case EBufferUsage::Uniform:
    return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  case EBufferUsage::Index:
    return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  case EBufferUsage::Storage:
    return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  case EBufferUsage::Indirect:
    return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
  case EBufferUsage::TransferSrc:
    return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  case EBufferUsage::TransferDst:
    return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  case EBufferUsage::None:
    return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
  }
}

constexpr auto ToVkImageUsageFlags(ETextureUsage usage) noexcept
    -> VkImageUsageFlags {
  if (usage == ETextureUsage::None) {
    return 0;
  }

  VkImageUsageFlags vkUsages = 0;

  if ((usage & ETextureUsage::Sampled) != ETextureUsage::None) {
    vkUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }

  if ((usage & ETextureUsage::ColorAttachment) != ETextureUsage::None) {
    vkUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }

  if ((usage & ETextureUsage::DepthStencilAttachment) != ETextureUsage::None) {
    vkUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }

  if ((usage & ETextureUsage::Storage) != ETextureUsage::None) {
    vkUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  }

  if ((usage & ETextureUsage::TransferSrc) != ETextureUsage::None) {
    vkUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  if ((usage & (ETextureUsage::ColorAttachment |
                ETextureUsage::DepthStencilAttachment)) !=
      ETextureUsage::None) {
    vkUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  AVALON_ASSERT_MSG(vkUsages != 0,
                    "[Vulkan] Image usage mapping resulted in 0 flags!");
  return vkUsages;
}

constexpr auto ToVkBufferUsageFlags(EBufferUsage usage) noexcept
    -> VkBufferUsageFlags {
  AVALON_ASSERT(usage != EBufferUsage::None);
  VkBufferUsageFlags vkUsages = 0;
  if ((usage & EBufferUsage::TransferDst) != EBufferUsage::None) {
    vkUsages |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if ((usage & EBufferUsage::TransferSrc) != EBufferUsage::None) {
    vkUsages |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
  if ((usage & EBufferUsage::Storage) != EBufferUsage::None) {
    vkUsages |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }
  if ((usage & EBufferUsage::Index) != EBufferUsage::None) {
    vkUsages |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }
  if ((usage & EBufferUsage::Uniform) != EBufferUsage::None) {
    vkUsages |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }
  if ((usage & EBufferUsage::Vertex) != EBufferUsage::None) {
    vkUsages |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }
  if ((usage & EBufferUsage::Indirect) != EBufferUsage::None) {
    vkUsages |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
  }

  AVALON_ASSERT(vkUsages != 0);
  return vkUsages;
}

constexpr auto ToVkMemoryPropertyFlags(EMemoryProperty property) noexcept
    -> VkMemoryPropertyFlags {
  AVALON_ASSERT(property != EMemoryProperty::None);

  VkMemoryPropertyFlags vkFlags = 0;
  if ((property & EMemoryProperty::DeviceLocal) != EMemoryProperty::None) {
    vkFlags |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  }
  if ((property & EMemoryProperty::HostVisible) != EMemoryProperty::None) {
    vkFlags |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  }
  if ((property & EMemoryProperty::HostCoherent) != EMemoryProperty::None) {
    vkFlags |= VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  }

  AVALON_ASSERT(vkFlags != 0);
  return vkFlags;
}

constexpr auto ToVkDescriptorType(EDescriptorType type) noexcept
    -> VkDescriptorType {
  switch (type) {
  case EDescriptorType::UniformBuffer:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  case EDescriptorType::StorageBuffer:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  case EDescriptorType::UniformBufferDynamic:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  case EDescriptorType::CombinedImageSampler:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  case EDescriptorType::SampledImage:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  case EDescriptorType::Sampler:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
  case EDescriptorType::StorageImage:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  case EDescriptorType::UniformTexelBuffer:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
  case EDescriptorType::StorageTexelBuffer:
    return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
  case EDescriptorType::StorageBufferDynamic:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
  case EDescriptorType::InputAttachment:
    return VkDescriptorType::VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  case EDescriptorType::AccelerationStructure:
    return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    break;
  }
}

constexpr auto ToVkCullMode(ECullMode cullMode) noexcept -> VkCullModeFlags {
  switch (cullMode) {
  case ECullMode::Back:
    return VK_CULL_MODE_BACK_BIT;
  case ECullMode::Front:
    return VK_CULL_MODE_FRONT_BIT;
  case ECullMode::FrontAndBack:
    return VK_CULL_MODE_FRONT_AND_BACK;
  case ECullMode::None:
    return VK_CULL_MODE_NONE;
  }
}

constexpr auto ToVkColorComponentFlags(EColorWriteMask mask) noexcept
    -> VkColorComponentFlags {

  if (mask == EColorWriteMask::None) {
    return 0;
  }

  VkColorComponentFlags vkFlags = 0;

  if ((static_cast<uint32_t>(mask) &
       static_cast<uint32_t>(EColorWriteMask::R)) != 0) {
    vkFlags |= VK_COLOR_COMPONENT_R_BIT;
  }
  if ((static_cast<uint32_t>(mask) &
       static_cast<uint32_t>(EColorWriteMask::G)) != 0) {
    vkFlags |= VK_COLOR_COMPONENT_G_BIT;
  }
  if ((static_cast<uint32_t>(mask) &
       static_cast<uint32_t>(EColorWriteMask::B)) != 0) {
    vkFlags |= VK_COLOR_COMPONENT_B_BIT;
  }
  if ((static_cast<uint32_t>(mask) &
       static_cast<uint32_t>(EColorWriteMask::A)) != 0) {
    vkFlags |= VK_COLOR_COMPONENT_A_BIT;
  }

  AVALON_ASSERT(vkFlags != 0 || mask == EColorWriteMask::None);

  return vkFlags;
}

constexpr auto ToVkShaderStageFlags(EShaderStage stage) noexcept
    -> VkShaderStageFlags {
  AVALON_ASSERT(stage != EShaderStage::None);
  VkShaderStageFlags vkStages = 0;
  if ((stage & EShaderStage::Vertex) != EShaderStage::None) {
    vkStages |= VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
  }
  if ((stage & EShaderStage::Fragment) != EShaderStage::None) {
    vkStages |= VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  if ((stage & EShaderStage::Compute) != EShaderStage::None) {
    vkStages |= vkStages |= VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
  }

  AVALON_ASSERT(vkStages != 0);
  return vkStages;
}

constexpr auto ToVkShaderStageBits(EShaderStage stage) noexcept
    -> VkShaderStageFlagBits {
  switch (stage) {
  case EShaderStage::None:
    return VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
  case EShaderStage::Vertex:
    return VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
  case EShaderStage::Fragment:
    return VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
  case EShaderStage::Compute:
    return VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
  case EShaderStage::All:
    return VkShaderStageFlagBits::VK_SHADER_STAGE_ALL;
  }
}

constexpr auto MapIntentToUsage(EAttachmentIntent intent) noexcept
    -> ETextureUsage {
  ETextureUsage usage = ETextureUsage::None;

  if ((intent & EAttachmentIntent::ReadOnly) != EAttachmentIntent::None) {
    usage |= ETextureUsage::Sampled;
  }

  if ((intent & EAttachmentIntent::WriteColor) != EAttachmentIntent::None) {
    usage |= ETextureUsage::ColorAttachment;
  }

  if ((intent & EAttachmentIntent::WriteDepth) != EAttachmentIntent::None) {
    usage |= ETextureUsage::DepthStencilAttachment;
  }

  if ((intent & EAttachmentIntent::CaptureSource) != EAttachmentIntent::None) {
    usage |= ETextureUsage::TransferSrc | ETextureUsage::ColorAttachment;
  }

  if ((intent & EAttachmentIntent::ComputeStorage) != EAttachmentIntent::None) {
    usage |= ETextureUsage::Storage;
  }

  return usage;
}

constexpr auto HandleVkError(VkResult result) noexcept -> ERhiResult {
  switch (result) {
  case VK_ERROR_DEVICE_LOST:
    return ERhiResult::DeviceLost;
    break;
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    return ERhiResult::OutOfMemory;
    break;
  case VK_ERROR_SURFACE_LOST_KHR:
    return ERhiResult::SurfaceLost;
    break;
  default:
    return ERhiResult::Unknown;
  }
}

constexpr StringView ToView(VkResult result) noexcept {
  switch (result) {
  case VK_SUCCESS:
    return "VK_SUCCESS";
  case VK_NOT_READY:
    return "VK_NOT_READY";
  case VK_TIMEOUT:
    return "VK_TIMEOUT";
  case VK_EVENT_SET:
    return "VK_EVENT_SET";
  case VK_EVENT_RESET:
    return "VK_EVENT_RESET";
  case VK_INCOMPLETE:
    return "VK_INCOMPLETE";
  case VK_ERROR_OUT_OF_HOST_MEMORY:
    return "VK_ERROR_OUT_OF_HOST_MEMORY";
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
  case VK_ERROR_INITIALIZATION_FAILED:
    return "VK_ERROR_INITIALIZATION_FAILED";
  case VK_ERROR_DEVICE_LOST:
    return "VK_ERROR_DEVICE_LOST";
  case VK_ERROR_MEMORY_MAP_FAILED:
    return "VK_ERROR_MEMORY_MAP_FAILED";
  case VK_ERROR_LAYER_NOT_PRESENT:
    return "VK_ERROR_LAYER_NOT_PRESENT";
  case VK_ERROR_EXTENSION_NOT_PRESENT:
    return "VK_ERROR_EXTENSION_NOT_PRESENT";
  case VK_ERROR_FEATURE_NOT_PRESENT:
    return "VK_ERROR_FEATURE_NOT_PRESENT";
  case VK_ERROR_INCOMPATIBLE_DRIVER:
    return "VK_ERROR_INCOMPATIBLE_DRIVER";
  case VK_ERROR_TOO_MANY_OBJECTS:
    return "VK_ERROR_TOO_MANY_OBJECTS";
  case VK_ERROR_FORMAT_NOT_SUPPORTED:
    return "VK_ERROR_FORMAT_NOT_SUPPORTED";
  case VK_ERROR_FRAGMENTED_POOL:
    return "VK_ERROR_FRAGMENTED_POOL";
  case VK_ERROR_UNKNOWN:
    return "VK_ERROR_UNKNOWN";
  case VK_ERROR_VALIDATION_FAILED:
    return "VK_ERROR_VALIDATION_FAILED";
  case VK_ERROR_OUT_OF_POOL_MEMORY:
    return "VK_ERROR_OUT_OF_POOL_MEMORY";
  case VK_ERROR_INVALID_EXTERNAL_HANDLE:
    return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
  case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
    return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
  case VK_ERROR_FRAGMENTATION:
    return "VK_ERROR_FRAGMENTATION";
  case VK_PIPELINE_COMPILE_REQUIRED:
    return "VK_PIPELINE_COMPILE_REQUIRED";
  case VK_ERROR_NOT_PERMITTED:
    return "VK_ERROR_NOT_PERMITTED";
  case VK_ERROR_SURFACE_LOST_KHR:
    return "VK_ERROR_SURFACE_LOST_KHR";
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
    return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
  case VK_SUBOPTIMAL_KHR:
    return "VK_SUBOPTIMAL_KHR";
  case VK_ERROR_OUT_OF_DATE_KHR:
    return "VK_ERROR_OUT_OF_DATE_KHR";
  case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
    return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
  case VK_ERROR_INVALID_SHADER_NV:
    return "VK_ERROR_INVALID_SHADER_NV";
  case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
    return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
  case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
  case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
  case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
  case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
  case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
    return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
  case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
    return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
  case VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT:
    return "VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT";
  case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
    return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
  case VK_THREAD_IDLE_KHR:
    return "VK_THREAD_IDLE_KHR";
  case VK_THREAD_DONE_KHR:
    return "VK_THREAD_DONE_KHR";
  case VK_OPERATION_DEFERRED_KHR:
    return "VK_OPERATION_DEFERRED_KHR";
  case VK_OPERATION_NOT_DEFERRED_KHR:
    return "VK_OPERATION_NOT_DEFERRED_KHR";
  case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
    return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
  case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
    return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
  case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
    return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
  case VK_PIPELINE_BINARY_MISSING_KHR:
    return "VK_PIPELINE_BINARY_MISSING_KHR";
  case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
    return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
  case VK_RESULT_MAX_ENUM:
    return "VK_RESULT_MAX_ENUM";
  }
}

} // namespace avalon::rhi
