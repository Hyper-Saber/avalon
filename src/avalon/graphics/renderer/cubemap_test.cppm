module;
#include <cstdint>
export module avalon.graphics:cubemap_test;

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
    m_cubeHandle =
        builder.Read("SkyboxPrefiltered"_id, EResourceUsage::ReadOnly,
                     EResourceUsage::ReadOnly, EShaderStage::Fragment);
    builder.WriteAttachment(kSceneColor, rhi::EResourceUsage::ColorAttachment);
  }

  void OnCompile(rhi::IRhi &rhi) override {
    auto &materialManager = GetMaterialManager();
    m_material = materialManager.Resolve(
        materialManager.TryGetMaterial("CubemapTest"_id));
  }

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto pipeline = m_material->GetOrCreatePipeline(
        context.rhi, context.pipelineRenderingInfo);

    auto handle = context.GetPhysicalTexture(m_cubeHandle);
    auto index = context.rhi.GetBindlessManager().RegisterTextureCube(handle);
    struct {
      uint32_t cubemap;
      uint32_t sampler;
      float paddings[kPushConstantFloatSize - 2];
    } customPush{
        .cubemap = index,
        .sampler = context.rhi.GetStaticSamplers().linearClamp,
    };

    cmd.BindPipeline(pipeline);
    cmd.PushConstants(EShaderStage::All, 0, sizeof(customPush), &customPush);
    cmd.Draw(3, 1, 0, 0);
  }

private:
  rhi::Extent2D m_extent;
  VirtualResourceHandle m_cubeHandle;
  Material *m_material = nullptr;
};

} // namespace avalon::graphics
