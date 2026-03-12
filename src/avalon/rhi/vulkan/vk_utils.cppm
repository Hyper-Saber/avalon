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

auto ToVkFormat(EFormat format) -> VkFormat {
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

auto ToVkLoadOp(EAttachmentLoadOp loadOp) -> VkAttachmentLoadOp {
  switch (loadOp) {
  case EAttachmentLoadOp::Load:
    return VK_ATTACHMENT_LOAD_OP_LOAD;
  case EAttachmentLoadOp::Clear:
    return VK_ATTACHMENT_LOAD_OP_CLEAR;
  case EAttachmentLoadOp::DontCare:
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  }
}

auto ToVkStoreOp(EAttachmentStoreOp storeOp) -> VkAttachmentStoreOp {
  switch (storeOp) {
  case EAttachmentStoreOp::Store:
    return VK_ATTACHMENT_STORE_OP_STORE;
  case EAttachmentStoreOp::DontCare:
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }
}

auto ToVkImageLayout(EResourceLayout layout) -> VkImageLayout {
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

auto ToVkPrimitiveTopology(EPrimitiveTopology topology) -> VkPrimitiveTopology {
  switch (topology) {
  case EPrimitiveTopology::PointList:
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  case EPrimitiveTopology::LineList:
    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  case EPrimitiveTopology::TriangleList:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  }
}

auto ToVkPolygonMode(EPolygonMode polygonMode) -> VkPolygonMode {
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

auto ToVkDepthCompareOp(EDepthCompareOp compareOp) -> VkCompareOp {
  switch (compareOp) {
  case EDepthCompareOp::Less:
    return VK_COMPARE_OP_LESS;
  case EDepthCompareOp::LessOrEqual:
    return VK_COMPARE_OP_LESS_OR_EQUAL;
  case EDepthCompareOp::Greater:
    return VK_COMPARE_OP_GREATER;
  case EDepthCompareOp::GreaterOrEqual:
    return VK_COMPARE_OP_GREATER_OR_EQUAL;
  case EDepthCompareOp::Equal:
    return VK_COMPARE_OP_EQUAL;
  case EDepthCompareOp::NotEqual:
    return VK_COMPARE_OP_NOT_EQUAL;
  }
}

auto ToVkBufferUsageBits(EBufferUsage usage) -> VkBufferUsageFlagBits {
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

auto ToVkBufferUsageFlags(EBufferUsage usage) -> VkBufferUsageFlags {
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

auto ToVkMemoryPropertyFlags(EMemoryProperty property)
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

auto ToVkDescriptorType(EDescriptorType type) -> VkDescriptorType {
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

auto ToVkCullMode(ECullMode cullMode) -> VkCullModeFlags {
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

auto ToVkShaderStageFlags(EShaderStage stage) -> VkShaderStageFlags {
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

auto ToVkShaderStageBits(EShaderStage stage) -> VkShaderStageFlagBits {
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

auto HandleVkError(VkResult result) -> ERhiResult {
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

StringView ToView(VkResult result) {
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
