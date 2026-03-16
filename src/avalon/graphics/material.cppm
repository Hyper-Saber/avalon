module;
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <debug/assert.hpp>
#include <utility>
export module avalon.graphics:material;
import avalon.shader;
import avalon.core;
import avalon.rhi;
import :utils;
import :mesh;

export namespace avalon::graphics {

class AVALON_GRAPHICS_API Material final : public RefCounted<Material> {
public:
  explicit Material(Handle<Shader> handle) : m_shaderHandle(handle) {
    auto shader = GetShaderManager().Resolve(handle);
    auto bindings = shader->GetDescriptorMetaData();

    for (const auto &binding : bindings) {
      auto blob = CreateEmptyBlob(binding.bufferSize);
      m_uniformBufferStates.EmplaceBack(binding.bindingPoint, std::move(blob));
    }

    InitializePropertyLayout(shader);
    BuildVertexInputState(shader);
  }

  template <typename T> void SetProperty(StringId nameHash, const T &value) {
    for (const auto &mapping : m_propertyLayout) {
      if (mapping.nameHash == nameHash) {
        UpdateInternal(mapping, value);
        return;
      }
    }
    AVALON_ASSERT(false);
  }

  auto GetPipelineCreateInfo() const -> rhi::PipelineCreateInfo {
    auto shader = GetShaderManager().Resolve(m_shaderHandle);

    rhi::PipelineCreateInfo info{
        .pushConstantRanges = shader->GetPushConstants(),
        .vertexInputAttributes = {m_vertexAttributes.GetData(),
                                  m_vertexAttributes.GetSize()},
        .vertexBindings = {m_vertexBindings.GetData(),
                           m_vertexBindings.GetSize()},
        .descriptorSetLayoutBindings = shader->GetDescriptorSetLayouts(),
        .stageInfos = shader->GetStageInfos(),
    };

    return info;
  }

  auto GetVertexLayout() const -> const VertexLayout & {
    return m_vertexLayout;
  }

private:
  struct UniformBufferState {
    uint32_t bindingPoint;
    BlobPtr data;
    bool isDirty = true;

    UniformBufferState(uint32_t bindingPoint, BlobPtr &&data)
        : bindingPoint(bindingPoint), data(std::move(data)) {}
  };

  struct PropertyMapping {
    StringId nameHash;
    uint32_t bufferIndex;
    uint32_t offset;
    uint32_t size;
  };

  template <typename T>
  void UpdateInternal(const PropertyMapping &mapping, const T &value) {
    AVALON_ASSERT_MSG(sizeof(T) <= mapping.size, "Property size mismatch!");

    auto &state = m_uniformBufferStates[mapping.bufferIndex];
    auto *dest = state.data->As<uint8_t>() + mapping.offset;

    if (std::memcmp(dest, &value, sizeof(T)) != 0) {
      std::memcpy(dest, &value, sizeof(T));
      state.isDirty = true;
    }
  }

  void InitializePropertyLayout(const Shader *shader) {
    auto members = shader->GetBufferMembers();
    m_propertyLayout.Reserve(members.GetSize());

    for (const auto &member : members) {
      for (uint32_t i = 0; i < m_uniformBufferStates.GetSize(); i++) {
        if (m_uniformBufferStates[i].bindingPoint == member.bindingPoint) {
          m_propertyLayout.PushBack({
              .nameHash = member.nameHash,
              .bufferIndex = i,
              .offset = member.offset,
              .size = member.size,
          });
          break;
        }
      }
    }
  }

  void BuildVertexInputState(const Shader *shader) {
    auto attributes = shader->GetInputAttributes();

    m_vertexAttributes.Reserve(attributes.GetSize());
    uint32_t currentOffset = 0;
    for (const auto &attr : attributes) {
      m_vertexAttributes.PushBack({
          .location = attr.location,
          .binding = 0,
          .format = attr.format,
          .semantic = attr.semantic,
          .offset = currentOffset,
      });
      auto size = rhi::GetFormatSize(attr.format);
      AVALON_ASSERT(size != rhi::kInvalidFormatSize);
      currentOffset += size;
    }

    m_vertexBindings.PushBack({
        .binding = 0,
        .stride = currentOffset,
        .isInstanceData = false,
    });

    std::sort(m_vertexAttributes.begin(), m_vertexAttributes.end(),
              [](auto &a, auto &b) { return a.location < b.location; });

    m_vertexLayout.attributes = {m_vertexAttributes.GetData(),
                                 m_vertexAttributes.GetSize()};
    m_vertexLayout.stride = ComputeStride();
  }

  uint32_t ComputeStride() {
    uint32_t stride = 0;
    auto attributes = m_vertexAttributes;
    for (const auto &attr : attributes) {
      stride += GetFormatSize(attr.format);
    }

    if constexpr (debug::kIsDebug) {
      uint32_t actualStride = 0;
      for (const auto &attr : attributes) {
        switch (attr.semantic) {
        case EVertexSemantic::Position:
          actualStride += sizeof(Vec3);
          break;
        case EVertexSemantic::TexCoord:
          actualStride += sizeof(Vec2);
          break;
        case EVertexSemantic::Color:
          actualStride += sizeof(Vec3);
          break;
        case EVertexSemantic::Normal:
          actualStride += sizeof(Vec3);
          break;
        default:
          break;
        }
      }

      AVALON_ASSERT(actualStride == stride);
    }

    return stride;
  }

  Handle<Shader> m_shaderHandle;
  Array<UniformBufferState> m_uniformBufferStates;
  Array<PropertyMapping> m_propertyLayout;
  Array<rhi::VertexInputAttribute> m_vertexAttributes;
  Array<rhi::VertexBinding> m_vertexBindings;
  VertexLayout m_vertexLayout;
};
} // namespace avalon::graphics
