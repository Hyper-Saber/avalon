module;
#include <cstdint>
#include <type_traits>
export module avalon.shader:serialization;

import avalon.core;
import avalon.rhi;

export namespace avalon::graphics {

constexpr StringView kDefaultVsEntryPointName = "VsMain";
constexpr StringView kDefaultFsEntryPointName = "FsMain";
constexpr StringView kDefaultCsEntryPointName = "CsMain";

constexpr uint32_t kMagicAVSB = 0x42535641;
constexpr int32_t kNoDefaultValue = -1;

#pragma pack(push, 4)

struct ShaderBlobHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t totalSize;
  uint32_t stageCount;
  uint32_t stageTableOffset;
  uint32_t mergedReflectionOffset;
  HashType checkSum;
};

struct ShaderStageHeader {
  rhi::EShaderStage stageType;
  StringId entryPointHash;
  uint32_t bytecodeOffset;
  uint32_t bytecodeSize;
  uint32_t reflectionOffset;
  uint32_t reflectionSize;
};

struct ShaderPushConstant {
  rhi::EShaderStage visibleStages;
  uint32_t offset;
  uint32_t size;
};

struct ShaderBufferMember {
  StringId nameHash;
  uint32_t offset;
  uint32_t size;
  uint32_t bindingPoint;
  uint32_t arrayStride;
  rhi::EFormat format;
  int32_t defaultValueOffset;
};

struct ShaderDescriptorBinding {
  StringId nameHash;
  uint32_t set;
  uint32_t bindingPoint;
  rhi::EDescriptorType type;
  rhi::EBufferUsage usage;
  rhi::EShaderStage visibleStages;
  uint32_t bufferSize;
  uint32_t memberCount;
  uint32_t memberOffset;
  uint32_t count;
};

struct ShaderInputAttribute {
  StringId nameHash;
  uint32_t location;
  rhi::EFormat format;
};

struct ShaderReflectionHeader {
  uint32_t pushConstantCount;
  uint32_t pushConstantTableOffset;
  uint32_t inputAttrCount;
  uint32_t inputAttrOffset;
  uint32_t descBindingCount;
  uint32_t descBindingTableOffset;
  uint32_t memberCount;
  uint32_t memberTableOffset;
  uint32_t defaultValuePoolSize;
  uint32_t defaultValuePoolOffset;
};

#pragma pack(pop)

static_assert(sizeof(ShaderBlobHeader) == 32, "ShaerBlobHeader size mismatch!");
static_assert(sizeof(ShaderStageHeader) == 28,
              "ShaderStageHeader size mismatch!");
static_assert(sizeof(ShaderBufferMember) == 32,
              "ShaderBufferMember size mismatch!");
static_assert(sizeof(ShaderPushConstant) == 12,
              "ShaderPushConstants size mismatch!");
static_assert(sizeof(ShaderDescriptorBinding) == 44,
              "ShaderResourceBinding size mismatch!");
static_assert(sizeof(ShaderInputAttribute) == 16,
              "ShaderInputAttribute size mismatch!");
static_assert(sizeof(ShaderReflectionHeader) == 40,
              "ShaderReflectionHeader size mismatch!");

static_assert(std::is_standard_layout_v<ShaderBlobHeader>,
              "Must be standard layout!");
static_assert(std::is_standard_layout_v<ShaderStageHeader>,
              "Must be standard layout!");
static_assert(std::is_standard_layout_v<ShaderBufferMember>,
              "Must be standard layout!");
static_assert(std::is_standard_layout_v<ShaderDescriptorBinding>,
              "Must be standard layout!");
static_assert(std::is_standard_layout_v<ShaderInputAttribute>,
              "Must be standard layout!");
static_assert(std::is_standard_layout_v<ShaderDescriptorBinding>,
              "Must be standard layout!");
static_assert(std::is_standard_layout_v<ShaderReflectionHeader>,
              "Must be standard layout!");

} // namespace avalon::graphics
