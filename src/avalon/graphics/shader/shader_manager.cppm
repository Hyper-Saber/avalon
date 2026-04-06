module;
#include <utility>
export module avalon.shader:shader_manager;

import avalon.core;
import :shader;
import :shader_compiler;
import :serialization;

export namespace avalon::graphics {

class ShaderManager final : public NonCopyable,
                            public mem::AutoDestroyable<ShaderManager> {
public:
  AVALON_SHADER_API ShaderManager(rhi::IRhi &rhi) : m_rhi(rhi) {
    m_compiler = MakeUnique<ShaderCompiler>();
  }

  AVALON_SHADER_API ~ShaderManager() override = default;

  auto AVALON_SHADER_API GetOrCreateShader(const Path &path) -> ShaderHandle {
    auto vPath = Path(vfs::kShaderFolderVirtualPath) / path;
    auto pathHash = vPath.GetId();

    if (auto handle = m_shaderCache.Get(pathHash)) {
      return *handle;
    }

    auto source = CreateEmptyBlob();
    auto result = vfs::GetVfs().ReadFile(vPath, source);
    if (result != vfs::EVfsError::None) {
      return {};
    }

    Path absPath;
    vfs::GetVfs().GetAbsolute(vPath, absPath);

    ShaderCompileDesc desc{
        .sourceCode = std::move(source),
        .filePath = absPath,
        .shaderStages =
            {
                {.entryPointName = kDefaultVsEntryPointName,
                 .shaderStage = EShaderStage::Vertex},
                {.entryPointName = kDefaultFsEntryPointName,
                 .shaderStage = EShaderStage::Fragment},
            },
    };

    auto binaryCode = m_compiler->Compile(desc);

    auto handle = m_shaderPool.Create(std::move(binaryCode));
    m_shaderCache.Insert(pathHash, handle);
    return handle;
  }

  auto AVALON_SHADER_API GetOrCreateComputeShader(
      const Path &path, const char *entryPoint = "CsMain") -> ShaderHandle {
    auto vPath = Path(vfs::kShaderFolderVirtualPath) / path;
    auto pathHash = vPath.GetId();
    if (auto handle = m_shaderCache.Get(pathHash))
      return *handle;

    auto source = CreateEmptyBlob();
    if (vfs::GetVfs().ReadFile(vPath, source) != vfs::EVfsError::None)
      return {};

    Path absPath;
    vfs::GetVfs().GetAbsolute(vPath, absPath);

    ShaderCompileDesc desc{
        .sourceCode = std::move(source),
        .filePath = absPath,
        .shaderStages = {{.entryPointName = entryPoint,
                          .shaderStage = EShaderStage::Compute}},
    };

    auto binaryCode = m_compiler->Compile(desc);
    auto handle = m_shaderPool.Create(std::move(binaryCode));
    m_shaderCache.Insert(pathHash, handle);
    CreateComputePipeline(handle);
    return handle;
  }

  auto AVALON_SHADER_API Resolve(ShaderHandle handle) -> const Shader * {
    return m_shaderPool.Resolve(handle);
  }

private:
  void CreateComputePipeline(ShaderHandle handle) {
    auto pShader = Resolve(handle);
    ComputePipelineCreateInfo info{
        .stageInfo = *pShader->GetComputeStageInfo(),
        .descriptorSetLayoutBindings = pShader->GetDescriptorSetLayouts(),
    };

    m_rhi.GetOrCreateComputePipeline(info);
  }

  rhi::IRhi &m_rhi;
  UniquePtr<ShaderCompiler> m_compiler;
  HashMap<StringId, ShaderHandle> m_shaderCache;
  mem::ResourcePool<Shader> m_shaderPool;
};
} // namespace avalon::graphics
