module;
#include <expected>
#include <filesystem>
#include <vector>

export module avalon.shader_compiler;
import avalon.core;

export namespace avalon::shader_compiler {

enum class EShaderStage {
  Vertex,
  Fragment,
  Compute,
};

enum class EShaderFeatureLevel { Level_6_0, Level_6_3, Level_6_6, Default };

struct CompileResult {
  std::vector<uint32_t> bytecode;
  std::string errorMessage;
  bool isSuccess = false;

  explicit operator bool() const { return isSuccess; }
};

class IShaderCompiler : public IPlugin {
public:
  virtual ~IShaderCompiler() = default;

  virtual auto
  CompileFromFile(const std::filesystem::path &path,
                  std::string_view entryPoint, EShaderStage stage,
                  EShaderFeatureLevel = EShaderFeatureLevel::Default)
      -> std::expected<CompileResult, EStatusCode> = 0;

  virtual auto
  CompileFromSource(const std::string &source, std::string_view entryPoint,
                    EShaderStage stage,
                    EShaderFeatureLevel level = EShaderFeatureLevel::Default)
      -> std::expected<CompileResult, EStatusCode> = 0;

  virtual auto LoadBytecode(const std::filesystem::path &path)
      -> std::expected<std::vector<uint32_t>, EStatusCode> = 0;
};
} // namespace avalon::shader_compiler
