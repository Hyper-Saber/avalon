module;
#include <algorithm>
#include <cstdint>
#include <debug/assert.hpp>

export module avalon.graphics:material;

import avalon.shader;
import avalon.core;
import avalon.rhi;
import :mesh;

export namespace avalon::graphics {

struct PropertyState {
  uint32_t bindingPoint;
  uint32_t bufferOffset;
  uint32_t size;
};

struct TextureState {
  uint32_t pushConstantTextureSlot;
  rhi::TextureHandle handle;
};

class AVALON_GRAPHICS_API Material final
    : public mem::AutoDestroyable<Material> {
public:
  explicit Material(ShaderHandle handle) : m_shaderHandle(handle) {}

  bool Initialize() {
    auto shader = GetShaderManager().Resolve(m_shaderHandle);
    if (!shader) {
      Error("[Graphics]: Failed to resolve shader for material!");
      return false;
    }
    InitializePropertyLayout(shader);
    InitializeTextureLayout(shader);
    BuildVertexInputState(shader);
    return true;
  }

  auto GetPropertyLayout() const noexcept
      -> const HashMap<StringId, PropertyState> & {
    return m_propertyMap;
  }

  auto GetTextureLayout() const noexcept
      -> const HashMap<StringId, TextureState> & {
    return m_textureMap;
  }

  auto GetPipeline(const rhi::PipelineRenderingInfo &renderingInfo) const
      -> rhi::PipelineHandle {
    auto hash = renderingInfo.GetHash();
    if (auto pCached = m_pipelines.Get(hash)) {
      return *pCached;
    }
    return rhi::PipelineHandle::Invalid();
  }

  auto GetVertexLayout() const noexcept -> const VertexLayout & {
    return m_cachedVertexLayout;
  }

  auto GetDataBlob() const noexcept -> const IBlob & { return *m_dataBlob; }

  auto GetDefaultPipeline() const { return m_defaultPipeline; }

  auto GetOrCreatePipeline(rhi::IRhi &rhi,
                           const rhi::PipelineRenderingInfo &renderingInfo)
      -> rhi::PipelineHandle {
    auto hash = renderingInfo.GetHash();
    if (auto pCached = m_pipelines.Get(hash)) {
      return *pCached;
    }

    auto pipelineCI = GetPipelineCreateInfo(renderingInfo);
    auto pipeline = rhi.GetOrCreatePipeline(pipelineCI);

    if (pipeline.IsValid()) {
      m_pipelines.Insert(hash, pipeline);
    }

    if (!m_defaultPipeline.IsValid())
      m_defaultPipeline = pipeline;
    return pipeline;
  }

  void SetTexture(StringId nameHash, rhi::TextureHandle handle) {
    auto texture = m_textureMap.Get(nameHash);
    texture->handle = handle;
  }

  template <typename T> void SetProperty(StringId nameHash, const T &value) {
    auto buffer = m_propertyMap.Get(nameHash);
    if (!buffer) {
      Error("[Material]: Property {} not exsit!", nameHash.Resolve());
      return;
    }
    m_dataBlob->Write(&value, buffer->bufferOffset, buffer->size);
  }

  void EnableDepthTest() { m_depthState.isDepthTestEnable = true; }
  void EnableDepthWrite() { m_depthState.isDepthWriteEnable = true; }
  void EnableStencilTest() { m_depthState.isStencilTestEnable = true; }
  void DisableDepthTest() { m_depthState.isDepthTestEnable = false; }
  void DisableDepthWrite() { m_depthState.isDepthWriteEnable = false; }
  void DisableStencilTest() { m_depthState.isStencilTestEnable = false; }
  void SetDepthComplieOp(rhi::ECompareOp op) {
    m_depthState.depthCompareOp = op;
  }
  void SetCullMode(rhi::ECullMode cullMode) {
    m_rasterState.cullMode = cullMode;
  }

  auto GetShader() const { return m_shaderHandle; }

private:
  auto GetPipelineCreateInfo(const rhi::PipelineRenderingInfo &renderingInfo)
      -> rhi::PipelineCreateInfo {
    auto shader = GetShaderManager().Resolve(m_shaderHandle);
    uint32_t attachmentCount = renderingInfo.colorAttachmentFormats.GetSize();

    m_colorBlendStates.Clear();
    m_colorBlendStates.Resize(attachmentCount);

    if (attachmentCount > 0) {
      m_colorBlendStates[0] = m_mainBlendState;
    }

    return rhi::PipelineCreateInfo{
        .renderingInfo = renderingInfo,
        .vertexInputAttributes = m_vertexAttributes,
        .vertexBindings = m_vertexBindings,
        .descriptorSetLayoutBindings = shader->GetDescriptorSetLayouts(),
        .stageInfos = shader->GetStageInfos(),
        .inputAssemblyState = m_inputAssembly,
        .rasterizationState = m_rasterState,
        .multisampleState = m_multisampleState,
        .depthStencilState = m_depthState,
        .colorBlendStates = m_colorBlendStates,
    };
  }

  void InitializePropertyLayout(const Shader *shader) {
    auto members = shader->GetBufferMembers();
    for (const auto &member : members) {
      Debug("[Material] Property: {}", member.nameHash.Resolve());
      m_propertyMap.Insert(member.nameHash,
                           {.bindingPoint = member.bindingPoint,
                            .bufferOffset = member.offset,
                            .size = member.size});
    }
    m_dataBlob = CreateEmptyBlob(sizeof(StandardMaterialData));
  }

  void InitializeTextureLayout(const Shader *shader) {
    auto members = shader->GetPushConstants();
    for (const auto &member : members) {
      m_textureMap.Insert(member.nameHash,
                          {.pushConstantTextureSlot = member.textureSlot});
    }
  }

  void BuildVertexInputState(const Shader *shader) {
    auto attributes = shader->GetInputAttributes();
    m_vertexAttributes.Clear();
    uint32_t currentOffset = 0;
    for (const auto &attr : attributes) {
      m_vertexAttributes.PushBack({
          .location = attr.location,
          .binding = 0,
          .format = attr.format,
          .semantic = attr.semantic,
          .offset = currentOffset,
      });
      currentOffset += rhi::GetFormatSize(attr.format);
    }

    m_vertexBindings.Clear();
    m_vertexBindings.PushBack(
        {.binding = 0, .stride = currentOffset, .isInstanceData = false});

    std::sort(m_vertexAttributes.begin(), m_vertexAttributes.end(),
              [](auto &a, auto &b) { return a.location < b.location; });

    m_cachedVertexLayout.stride = currentOffset;
    m_cachedVertexLayout.attributes = Span<const rhi::VertexInputAttribute>(
        m_vertexAttributes.GetData(), m_vertexAttributes.GetSize());
  }

  ShaderHandle m_shaderHandle;
  rhi::DescriptorSetHandle m_descriptorSet;

  rhi::InputAssemblyState m_inputAssembly{};
  rhi::RasterizationState m_rasterState{};
  rhi::DepthStencilState m_depthState{};

  rhi::MultisampleState m_multisampleState{};
  rhi::ColorBlendState m_mainBlendState{};

  BlobPtr m_dataBlob;
  HashMap<StringId, PropertyState> m_propertyMap;
  HashMap<StringId, TextureState> m_textureMap;

  Array<rhi::VertexInputAttribute> m_vertexAttributes;
  Array<rhi::VertexBinding> m_vertexBindings;
  Array<rhi::ColorBlendState> m_colorBlendStates;

  HashMap<HashType, rhi::PipelineHandle> m_pipelines;
  rhi::PipelineHandle m_defaultPipeline;

  VertexLayout m_cachedVertexLayout{};
};

} // namespace avalon::graphics
