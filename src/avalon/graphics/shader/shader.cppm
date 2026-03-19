module;
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <debug/assert.hpp>
#include <utility>
export module avalon.shader:shader;

import avalon.core;
import avalon.rhi;
import :serialization;
import :utils;

export namespace avalon::graphics {

class AVALON_SHADER_API Shader final : public NonCopyable,
                                       public mem::AutoDestroyable<Shader> {
public:
  bool Initialize() {
    const std::byte *pData = m_shaderBlob->ConstAs<std::byte>();
    auto pMainHeader = reinterpret_cast<const ShaderBlobHeader *>(pData);
    AVALON_ASSERT_MSG(pMainHeader->magic == kMagicAVSB &&
                          pMainHeader->stageCount > 0,
                      "Invalid shader blob!");

    ParseShaderStages(pData, pMainHeader);

    auto pReflBase = reinterpret_cast<const ShaderReflectionHeader *>(
        pData + pMainHeader->mergedReflectionOffset);
    ParseReflectionView(pReflBase);
    ParseVertexInput();
    ParseDescriptorSetLayouts();
    ParsePushConstants(reinterpret_cast<const std::byte *>(pReflBase));
    return true;
  }

  explicit Shader(BlobPtr &&shaderBlob) : m_shaderBlob(std::move(shaderBlob)) {
    AVALON_ASSERT(m_shaderBlob->GetData() != nullptr)
  }

  auto GetPushConstantStageMask() const {
    if (m_pushConstantRanges.GetSize() == 0)
      return EShaderStage::None;
    return m_pushConstantRanges[0].visibleStages;
  }

  auto GetDescriptorMetaData() const noexcept
      -> Span<const ShaderDescriptorBinding> {
    return {m_reflection.pBindings, m_reflection.pHeader->descBindingCount};
  }

  auto GetDescriptorMetaData(uint32_t bindingPoint) const
      -> const ShaderDescriptorBinding & {
    const auto *begin = m_reflection.pBindings;

    for (uint32_t i = 0; i < m_reflection.pHeader->descBindingCount; i++) {
      if (begin[i].bindingPoint == bindingPoint) {
        return begin[i];
      }
    }

    AVALON_ASSERT_MSG(
        false, "Descriptor binding point not found in shader reflection!");
    return begin[0];
  }

  auto GetPushConstants() const -> Span<const rhi::PushConstantRange> {
    return {m_pushConstantRanges.GetData(), m_pushConstantRanges.GetSize()};
  }

  auto GetInputAttributes() const noexcept
      -> Span<const rhi::VertexInputAttribute> {
    return {m_vertexAttributes.GetData(), m_vertexAttributes.GetSize()};
  }

  auto GetDescriptorSetLayouts() const noexcept
      -> Span<const rhi::DescriptorSetLayoutBinding> {
    return {m_descriptorBindings.GetData(), m_descriptorBindings.GetSize()};
  }

  auto GetStageInfos() const noexcept -> Span<const rhi::ShaderStageInfo> {
    return {m_stageInfos.GetData(), m_stageInfos.GetSize()};
  }

  auto GetBufferMembers() const noexcept -> Span<const ShaderBufferMember> {
    return {m_reflection.pBufferMembers, m_reflection.pHeader->memberCount};
  }

  auto FindBufferMember(const ShaderDescriptorBinding &binding,
                        StringId nameHash) const -> const ShaderBufferMember * {
    const auto start = binding.memberOffset;
    const auto end = start + binding.memberCount;

    for (auto i = start; i < end; i++) {
      if (m_reflection.pBufferMembers[i].nameHash == nameHash) {
        return &m_reflection.pBufferMembers[i];
      }
    }

    return nullptr;
  }

  auto FindBufferMember(StringId nameId) const -> const ShaderBufferMember * {
    for (uint32_t i = 0; i < m_reflection.pHeader->memberCount; i++) {
      if (m_reflection.pBufferMembers[i].nameHash == nameId) {
        return &m_reflection.pBufferMembers[i];
      }
    }

    AVALON_ASSERT_MSG(false, "Shader buffer member not found!");
    return nullptr;
  }

private:
  void ParseShaderStages(const std::byte *pData,
                         const ShaderBlobHeader *pMainHeader) {
    auto pStageTable = reinterpret_cast<const ShaderStageHeader *>(
        pData + pMainHeader->stageTableOffset);

    m_stageInfos.Reserve(pMainHeader->stageCount);
    for (uint32_t i = 0; i < pMainHeader->stageCount; i++) {
      const auto &header = pStageTable[i];

      m_stageInfos.PushBack({
          .stage = header.stageType,
          .entryName = ToEntryNameView(header.stageType),
          .shaderCode = CreateViewBlob(pData + header.bytecodeOffset,
                                       header.bytecodeSize),
      });
    }
  }

  void ParseReflectionView(const ShaderReflectionHeader *pReflBase) {
    m_reflection.pHeader = pReflBase;

    auto pBase = reinterpret_cast<const std::byte *>(pReflBase);

    m_reflection.pAttributes = reinterpret_cast<const ShaderInputAttribute *>(
        pBase + m_reflection.pHeader->inputAttrOffset);
    m_reflection.pBindings = reinterpret_cast<const ShaderDescriptorBinding *>(
        pBase + m_reflection.pHeader->descBindingTableOffset);
    m_reflection.pBufferMembers = reinterpret_cast<const ShaderBufferMember *>(
        pBase + m_reflection.pHeader->memberTableOffset);
  }

  void ParseVertexInput() {
    uint32_t attrCount = m_reflection.pHeader->inputAttrCount;
    m_vertexAttributes.Reserve(attrCount);

    for (uint32_t i = 0; i < attrCount; i++) {
      const auto &attr = m_reflection.pAttributes[i];
      m_vertexAttributes.PushBack({
          .location = attr.location,
          .format = attr.format,
          .semantic = attr.semantic,
      });
    }
  }

  void ParseDescriptorSetLayouts() {
    uint32_t bindingCount = m_reflection.pHeader->descBindingCount;
    m_descriptorBindings.Reserve(bindingCount);

    for (uint32_t i = 0; i < bindingCount; i++) {
      const auto &binding = m_reflection.pBindings[i];
      m_descriptorBindings.PushBack({
          .nameHash = binding.nameHash,
          .binding = binding.bindingPoint,
          .set = binding.set,
          .type = binding.type,
          .visibleStages = binding.visibleStages,
          .count = binding.count,
      });
    }

    std::sort(m_descriptorBindings.begin(), m_descriptorBindings.end(),
              [](auto &a, auto &b) {
                if (a.set != b.set)
                  return a.set < b.set;
                return a.binding < b.binding;
              });
  }

  void ParsePushConstants(const std::byte *pReflBase) {
    if (m_reflection.pHeader->pushConstantCount == 0)
      return;

    auto pPushTable = reinterpret_cast<const ShaderPushConstant *>(
        pReflBase + m_reflection.pHeader->pushConstantTableOffset);

    for (uint32_t i = 0; i < m_reflection.pHeader->pushConstantCount; i++) {
      const auto &pushConstants = pPushTable[i];

      m_pushConstantRanges.PushBack({
          .visibleStages = pushConstants.visibleStages,
          .offset = pushConstants.offset,
          .size = pushConstants.size,
      });
    }
  }

  struct ReflectionView {
    const ShaderReflectionHeader *pHeader;
    const ShaderDescriptorBinding *pBindings;
    const ShaderInputAttribute *pAttributes;
    const ShaderBufferMember *pBufferMembers;
  };

  const BlobPtr m_shaderBlob;
  ReflectionView m_reflection;
  Array<rhi::ShaderStageInfo> m_stageInfos;
  Array<rhi::VertexInputAttribute> m_vertexAttributes;
  Array<rhi::DescriptorSetLayoutBinding> m_descriptorBindings;
  Array<rhi::PushConstantRange> m_pushConstantRanges;
};

using ShaderHandle = Handle<Shader>;
} // namespace avalon::graphics
