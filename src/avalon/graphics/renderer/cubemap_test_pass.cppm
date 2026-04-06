module;
#include <debug/assert.hpp>
export module avalon.graphics:cubemap_test_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

class CubemapTestPass final : public RenderPass<CubemapTestPass> {
public:
  void Setup(RenderGraphBuilder &builder) override {
    m_outputHandle =
        builder.Write(kSwapchainColor, rhi::EResourceUsage::Present);
    m_inputHandle = builder.Read(kSkyboxCube, rhi::EResourceUsage::ReadOnly);
  }

  void OnCompile(rhi::IRhi &rhi) override {
    auto &mm = GetMaterialManager();
    auto handle = mm.TryGetMaterial("CubemapTest"_id);
    m_material = mm.Resolve(handle);
    m_material->DisableDepthTest();
    m_material->DisableDepthWrite();
    auto instance = mm.Resolve(mm.GetOrCreateDefaultMaterialInstance(handle));
    instance->SetProperty("uMaterials.sampler"_id,
                          rhi.GetStaticSamplers().linearClamp);
    m_materialInstance = instance;
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    if (!m_material || !m_materialInstance)
      return;

    auto pipeline = m_material->GetOrCreatePipeline(
        context.rhi, context.pipelineRenderingInfo);
    cmd.BindPipeline(pipeline);

    auto textureSlot = m_materialInstance->GetTextureSlot("cubeTexture"_id);
    if (textureSlot != kInvalidTextureSlot) {
      auto handle = context.GetPhysicalTexture(m_inputHandle);
      auto textureIndex =
          context.rhi.GetBindlessManager().RegisterTextureCube(handle);
      StandardPushConstant constant;
      constant.customSlots[textureSlot] = textureIndex;

      cmd.PushConstants(rhi::EShaderStage::All, 0, sizeof(StandardPushConstant),
                        &constant);
    }

    cmd.Draw(3, 1, 0, 0);
  }

private:
  VirtualResourceHandle m_inputHandle;
  VirtualResourceHandle m_outputHandle;
  Material *m_material = nullptr;
  MaterialInstance *m_materialInstance = nullptr;
};

} // namespace avalon::graphics
