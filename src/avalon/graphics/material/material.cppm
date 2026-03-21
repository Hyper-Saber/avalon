module;
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <debug/assert.hpp>
export module avalon.graphics:material;
import avalon.shader;
import avalon.core;
import avalon.rhi;
import :mesh;
import :context;

export namespace avalon::graphics {

struct UniformBufferState {
  StringId nameHash;
  uint32_t bindingPoint;
  size_t bufferOffset;
  size_t size;
  bool isDirty = true;

  UniformBufferState(StringId nameHash, uint32_t bindingPoint, size_t offset,
                     size_t size)
      : nameHash(nameHash), bindingPoint(bindingPoint), bufferOffset(offset),
        size(size) {}
};

struct PropertyMapping {
  StringId nameHash;
  uint32_t bufferIndex;
  uint32_t memberOffset;
  uint32_t size;
};

class AVALON_GRAPHICS_API Material final
    : public mem::AutoDestroyable<Material> {
public:
  explicit Material(ShaderHandle handle) : m_shaderHandle(handle) {}

  bool Initialize() {
    auto shader = GetShaderManager().Resolve(m_shaderHandle);
    if (!shader) {
      Error("[Graphics]: Failed to initialize material!");
      return false;
    }

    auto alignment =
        GraphicsContext::Get()
            .deviceCapabilities.limits.minUniformBufferOffsetAlignment;

    auto bindings = shader->GetDescriptorMetaData();

    size_t currentOffset = 0;
    for (const auto &binding : bindings) {
      if (binding.set == 0)
        continue;
      switch (binding.type) {
      case rhi::EDescriptorType::UniformBuffer:
      case rhi::EDescriptorType::UniformBufferDynamic: {
        currentOffset = mem::AlignUp(currentOffset, alignment);
        m_uniformBufferStates.PushBack(
            UniformBufferState(binding.nameHash, binding.bindingPoint,
                               currentOffset, binding.bufferSize));
        currentOffset += binding.bufferSize;
        break;
      }
      default:
        break;
      }
    }

    m_dataBlob = CreateEmptyBlob(currentOffset);

    InitializePropertyLayout(shader);
    BuildVertexInputState(shader);
    return true;
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

  auto GetInitialBufferStates() const -> const Array<UniformBufferState> & {
    return m_uniformBufferStates;
  }

  auto GetDataBlob() const -> const IBlob & { return *m_dataBlob.Get(); }

  auto GetPropertyLayout() const -> const Array<PropertyMapping> & {
    return m_propertyLayout;
  }

  auto GetSHader() const { return m_shaderHandle; }

  auto GetPipelineCreateInfo(uint32_t attachmentCount = 1)
      -> rhi::PipelineCreateInfo {
    auto shader = GetShaderManager().Resolve(m_shaderHandle);

    m_colorBlendStates.Clear();
    m_colorBlendStates.ResizeUnInitialized(attachmentCount);

    m_colorBlendStates[0] = m_mainBlendState;
    for (uint32_t i = 1; i < attachmentCount; i++) {
      m_colorBlendStates[i] = rhi::ColorBlendState{
          .isEnable = false,
          .writeMask = rhi::EColorWriteMask::All,
      };
    }

    rhi::PipelineCreateInfo info{
        .pushConstantRanges = shader->GetPushConstants(),
        .vertexInputAttributes = {m_vertexAttributes.GetData(),
                                  m_vertexAttributes.GetSize()},
        .vertexBindings = {m_vertexBindings.GetData(),
                           m_vertexBindings.GetSize()},
        .descriptorSetLayoutBindings = shader->GetDescriptorSetLayouts(),
        .stageInfos = shader->GetStageInfos(),
        .colorBlendStates = m_colorBlendStates,
    };

    return info;
  }

  auto GetVertexLayout() const -> const VertexLayout & {
    return m_vertexLayout;
  }

private:
  template <typename T>
  void UpdateInternal(const PropertyMapping &mapping, const T &value) {
    AVALON_ASSERT_MSG(sizeof(T) == mapping.size, "Property size mismatch!");

    auto &state = m_uniformBufferStates[mapping.bufferIndex];

    state.isDirty = m_dataBlob->Write(
        &value, state.bufferOffset + mapping.memberOffset, mapping.size);
  }

  void InitializePropertyLayout(const Shader *shader) {
    auto members = shader->GetBufferMembers();
    m_propertyLayout.Reserve(m_uniformBufferStates.GetSize());

    for (const auto &member : members) {
      for (uint32_t i = 0; i < m_uniformBufferStates.GetSize(); i++) {
        if (m_uniformBufferStates[i].bindingPoint == member.bindingPoint) {
          m_propertyLayout.PushBack({
              .nameHash = member.nameHash,
              .bufferIndex = i,
              .memberOffset = member.offset,
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

  ShaderHandle m_shaderHandle;
  BlobPtr m_dataBlob;
  Array<UniformBufferState> m_uniformBufferStates;
  Array<PropertyMapping> m_propertyLayout;
  Array<rhi::VertexInputAttribute> m_vertexAttributes;
  Array<rhi::VertexBinding> m_vertexBindings;
  VertexLayout m_vertexLayout;

  Array<rhi::ColorBlendState> m_colorBlendStates;
  rhi::ColorBlendState m_mainBlendState;
};

using MaterialHandle = Handle<Material>;

} // namespace avalon::graphics
