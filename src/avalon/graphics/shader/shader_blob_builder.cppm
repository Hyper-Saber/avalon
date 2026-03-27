module;
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <debug/assert.hpp>
#include <utility>

export module avalon.shader:shader_blob_builder;
import avalon.core;
import :serialization;

namespace avalon::graphics {

struct ReflectionData {
  Array<ShaderCustomPushConstantTextureSlot> pushConstantMembers;
  Array<ShaderInputAttribute> inputAttributes;
  Array<ShaderDescriptorBinding> descBindings;
  Array<ShaderBufferMember> bufferMembers;
  Array<std::byte> defaultValuePool;
};

class ShaderBlobBuilder {
public:
  ShaderBlobBuilder() { m_buffer.Resize(sizeof(ShaderBlobHeader)); }

  ShaderBlobBuilder &AddStage(rhi::EShaderStage stage, StringId entryPointHash,
                              const void *bytecode, uint32_t size,
                              const ReflectionData &data) {

    m_pendingReflections.PushBack(data);

    ShaderStageHeader header = {
        .stageType = stage,
        .entryPointHash = entryPointHash,
    };

    header.bytecodeOffset = static_cast<uint32_t>(m_buffer.GetSize());
    header.bytecodeSize = size;
    AppendRaw(bytecode, size);
    Align(4);

    header.reflectionOffset = static_cast<uint32_t>(m_buffer.GetSize());
    WriteReflectionBlock(data);
    header.reflectionSize =
        static_cast<uint32_t>(m_buffer.GetSize()) - header.reflectionOffset;
    Align(4);

    m_stages.PushBack(header);

    return *this;
  }

  Array<std::byte> Build(uint32_t version = 1) {
    ReflectionData mergedData = MergeStages();

    uint32_t mergedReflOffset = static_cast<uint32_t>(m_buffer.GetSize());
    WriteReflectionBlock(mergedData);
    Align(4);

    uint32_t tableOffset = static_cast<uint32_t>(m_buffer.GetSize());
    for (const auto &header : m_stages) {
      AppendRaw(&header, sizeof(ShaderStageHeader));
    }

    ShaderBlobHeader mainHeader{
        .magic = kMagicAVSB,
        .version = version,
        .totalSize = static_cast<uint32_t>(m_buffer.GetSize()),
        .stageCount = static_cast<uint32_t>(m_stages.GetSize()),
        .stageTableOffset = tableOffset,
        .mergedReflectionOffset = mergedReflOffset,
        .checkSum =
            Hash::Compute(m_buffer.GetData() + sizeof(ShaderBlobHeader),
                          m_buffer.GetSize() - sizeof(ShaderBlobHeader))};

    std::memcpy(m_buffer.GetData(), &mainHeader, sizeof(ShaderBlobHeader));

    return std::move(m_buffer);
  }

private:
  void WriteReflectionBlock(const ReflectionData &data) {
    const auto &pushConstantRanges = data.pushConstantMembers;
    const auto &inputAttributes = data.inputAttributes;
    const auto &descBindings = data.descBindings;
    const auto &bufferMembers = data.bufferMembers;
    const auto &defaultValuePool = data.defaultValuePool;

    uint32_t relativePushConstantOffset = sizeof(ShaderReflectionHeader);
    uint32_t relativeInputOffset =
        relativePushConstantOffset +
        static_cast<uint32_t>(sizeof(ShaderCustomPushConstantTextureSlot) *
                              pushConstantRanges.GetSize());
    uint32_t relativeDescBindingOffset =
        relativeInputOffset +
        static_cast<uint32_t>(sizeof(ShaderInputAttribute) *
                              inputAttributes.GetSize());
    uint32_t relativeMemberOffset =
        relativeDescBindingOffset +
        static_cast<uint32_t>(sizeof(ShaderDescriptorBinding) *
                              descBindings.GetSize());
    uint32_t relativeValuePoolOffset =
        relativeMemberOffset +
        sizeof(ShaderBufferMember) * bufferMembers.GetSize();

    ShaderReflectionHeader reflHeader{
        .pushConstantCount =
            static_cast<uint32_t>(pushConstantRanges.GetSize()),
        .pushConstantTableOffset = relativePushConstantOffset,
        .inputAttrCount = static_cast<uint32_t>(inputAttributes.GetSize()),
        .inputAttrOffset = relativeInputOffset,
        .descBindingCount = static_cast<uint32_t>(descBindings.GetSize()),
        .descBindingTableOffset = relativeDescBindingOffset,
        .memberCount = static_cast<uint32_t>(bufferMembers.GetSize()),
        .memberTableOffset = relativeMemberOffset,
        .defaultValuePoolSize =
            static_cast<uint32_t>(defaultValuePool.GetSize()),
        .defaultValuePoolOffset = relativeValuePoolOffset,
    };

    AppendRaw(&reflHeader, sizeof(ShaderReflectionHeader));
    if (!pushConstantRanges.IsEmpty()) {
      AppendRaw(pushConstantRanges.GetData(),
                pushConstantRanges.GetSize() *
                    sizeof(ShaderCustomPushConstantTextureSlot));
    }
    if (!inputAttributes.IsEmpty())
      AppendRaw(inputAttributes.GetData(),
                inputAttributes.GetSize() * sizeof(ShaderInputAttribute));
    if (!descBindings.IsEmpty())
      AppendRaw(descBindings.GetData(),
                descBindings.GetSize() * sizeof(ShaderDescriptorBinding));
    if (!bufferMembers.IsEmpty())
      AppendRaw(bufferMembers.GetData(),
                bufferMembers.GetSize() * sizeof(ShaderBufferMember));
    if (!defaultValuePool.IsEmpty())
      AppendRaw(defaultValuePool.GetData(),
                defaultValuePool.GetSize() * sizeof(std::byte));
  }

  ReflectionData MergeStages() {
    ReflectionData merged;

    for (const auto &stageData : m_pendingReflections) {
      for (const auto &pushConstant : stageData.pushConstantMembers) {
        // ShaderCustomPushConstantMember *pExistPushConstant = nullptr;
        // for (auto &mergedPushConstant : merged.pushConstantMembers) {
        //   if (mergedPushConstant.offset == pushConstant.offset &&
        //       mergedPushConstant.size == pushConstant.size) {
        //     pExistPushConstant = &mergedPushConstant;
        //     break;
        //   }
        // }
        // if (pExistPushConstant) {
        //   pExistPushConstant->visibleStages |= pushConstant.visibleStages;
        // } else {
        merged.pushConstantMembers.PushBack(pushConstant);
        // }
      }

      for (const auto &attr : stageData.inputAttributes) {
        bool IsExists = false;

        for (const auto &mergedAttr : merged.inputAttributes) {
          if (mergedAttr.location == attr.location) {
            IsExists = true;
            break;
          }
        }
        if (!IsExists)
          merged.inputAttributes.PushBack(attr);
      }

      for (const auto &binding : stageData.descBindings) {
        ShaderDescriptorBinding *pExistBinding = nullptr;
        for (auto &mergedBinding : merged.descBindings) {
          if (mergedBinding.set == binding.set &&
              mergedBinding.bindingPoint == binding.bindingPoint) {
            pExistBinding = &mergedBinding;
            break;
          }
        }
        if (pExistBinding) {
          pExistBinding->visibleStages |= binding.visibleStages;
          AVALON_ASSERT(pExistBinding->usage == binding.usage);
          AVALON_ASSERT(pExistBinding->type == binding.type);
        } else {
          merged.descBindings.PushBack(binding);
        }
      }

      for (const auto &member : stageData.bufferMembers) {
        ShaderBufferMember *pExistMember = nullptr;
        for (auto &mergedMember : merged.bufferMembers) {
          if (mergedMember.nameHash == member.nameHash &&
              mergedMember.bindingPoint == member.bindingPoint) {
            pExistMember = &mergedMember;
            break;
          }
        }
        if (!pExistMember) {
          ShaderBufferMember newMember = member;
          if (member.defaultValueOffset != kNoDefaultValue) {
            const void *pSrc = stageData.defaultValuePool.GetData() +
                               member.defaultValueOffset;
            newMember.defaultValueOffset =
                static_cast<uint32_t>(merged.defaultValuePool.GetSize());
            merged.defaultValuePool.PushBackRaw(pSrc, newMember.size);
          }
          merged.bufferMembers.PushBack(newMember);

        } else {
          if (member.defaultValueOffset != kNoDefaultValue &&
              pExistMember->defaultValueOffset == kNoDefaultValue) {
            const void *pSrc = stageData.defaultValuePool.GetData() +
                               member.defaultValueOffset;
            pExistMember->defaultValueOffset =
                static_cast<uint32_t>(merged.defaultValuePool.GetSize());
            merged.defaultValuePool.PushBackRaw(pSrc, member.size);
          }
        }
      }
    }
    return merged;
  }

  void AppendRaw(const void *data, size_t size) {
    m_buffer.PushBackRaw(data, size);
  }

  void Align(size_t alignment) {
    size_t current = m_buffer.GetSize();
    size_t padded = (current + alignment - 1) & ~(alignment - 1);
    if (padded > current)
      m_buffer.Resize(padded);
  }

  Array<std::byte> m_buffer;
  Array<ShaderStageHeader> m_stages;
  Array<ReflectionData> m_pendingReflections;
};
} // namespace avalon::graphics
