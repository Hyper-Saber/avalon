#include <dxcapi.h>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

import avalon.shader_compiler;
import avalon.core;

namespace avalon::shader_compiler {

template <typename T> class DxcPtr {
public:
  DxcPtr() : m_ptr(nullptr) {}
  DxcPtr(T *p) : m_ptr(p) {}
  ~DxcPtr() {
    if (m_ptr)
      m_ptr->Release();
  }

  DxcPtr(const DxcPtr &) = delete;
  DxcPtr &operator=(const DxcPtr &) = delete;

  DxcPtr(DxcPtr &&other) noexcept : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
  }
  DxcPtr &operator=(DxcPtr &&other) noexcept {
    if (this != &other) {
      if (m_ptr)
        m_ptr->Release();
      m_ptr = other.m_ptr;
      other.m_ptr = nullptr;
    }
    return *this;
  }

  T *Get() const { return m_ptr; }
  T **GetAddressOf() { return &m_ptr; }
  T *operator->() const { return m_ptr; }
  explicit operator bool() const { return m_ptr != nullptr; }

private:
  T *m_ptr;
};

static std::wstring GetTargetProfile(EShaderStage stage,
                                     EShaderFeatureLevel level) {
  std::wstring version = L"6_0";
  if (level == EShaderFeatureLevel::Level_6_3)
    version = L"6_3";
  else if (level == EShaderFeatureLevel::Level_6_6)
    version = L"6_6";

  switch (stage) {
  case EShaderStage::Vertex:
    return L"vs_" + version;
  case EShaderStage::Fragment:
    return L"ps_" + version;
  case EShaderStage::Compute:
    return L"cs" + version;
    break;
  }

  return L"lib_" + version;
}

class ShaderCompiler final : public IShaderCompiler {
public:
  auto OnLoad() -> std::expected<void, EStatusCode> override { return {}; }

  void Cleanup() override {}

  auto CompileFromFile(const std::filesystem::path &path,
                       std::string_view entryPoint, EShaderStage stage,
                       EShaderFeatureLevel level)
      -> std::expected<CompileResult, EStatusCode> override {
    auto fileData = avalon::GetContext().pVfs->ReadFile(path);
    if (!fileData)
      return std::unexpected(EStatusCode::FileNotFound);

    std::string source(
        reinterpret_cast<const char *>(fileData.value()->GetData()),
        fileData.value()->GetSize());

    return CompileFromSource(source, entryPoint, stage, level);
  }

  auto CompileFromSource(const std::string &source, std::string_view entryPoint,
                         EShaderStage stage, EShaderFeatureLevel level)
      -> std::expected<CompileResult, EStatusCode> override {
    DxcPtr<IDxcUtils> pUtils;
    DxcPtr<IDxcCompiler3> pCompiler;

    HRESULT hr =
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf()));
    if (FAILED(hr)) {
      avalon::Error("[shader compiler]: failed to create dxc utils.");
      return std::unexpected(EStatusCode::InternalError);
    }

    hr = DxcCreateInstance(CLSID_DxcCompiler,
                           IID_PPV_ARGS(pCompiler.GetAddressOf()));
    if (FAILED(hr)) {
      avalon::Error("[shader compiler]: failed to create dxc compiler.");
      return std::unexpected(EStatusCode::InternalError);
    }

    auto wEntryPoint = std::wstring(entryPoint.begin(), entryPoint.end());
    auto wProfile = GetTargetProfile(stage, level);

    std::vector<LPCWSTR> wArgs;
    wArgs.push_back(L"-E");
    wArgs.push_back(wEntryPoint.c_str());
    wArgs.push_back(L"-T");
    wArgs.push_back(wProfile.c_str());
    if constexpr (avalon::kIsLinux) {
      wArgs.push_back(L"-spirv");
      wArgs.push_back(L"-fspv-target-env=universal1.5");
    }
    wArgs.push_back(L"-fspv-reflect");
    wArgs.push_back(L"-D");
    wArgs.push_back(L"VK_LOCATION(n)=[[vk::location(n)]]");

    DxcBuffer sourceBuffer{source.data(), source.size(), DXC_CP_UTF8};
    DxcPtr<IDxcResult> pResult;
    hr = pCompiler->Compile(&sourceBuffer, wArgs.data(),
                            static_cast<uint32_t>(wArgs.size()), nullptr,
                            IID_PPV_ARGS(pResult.GetAddressOf()));
    if (FAILED(hr)) {
      avalon::Error("[shader compiler]: failed to compile shader. Error: "
                    "internal error.");
      return std::unexpected(EStatusCode::InternalError);
    }
    CompileResult finalResult;
    DxcPtr<IDxcBlobUtf8> pErrors;
    pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()),
                       nullptr);
    if (pErrors && pErrors->GetStringLength() > 0) {
      finalResult.errorMessage = pErrors->GetStringPointer();
    }
    HRESULT status;
    pResult->GetStatus(&status);
    if (FAILED(status)) {
      finalResult.isSuccess = false;
      return finalResult;
    }

    DxcPtr<IDxcBlob> pByteCode;
    pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pByteCode.GetAddressOf()),
                       nullptr);
    if (pByteCode) {
      finalResult.isSuccess = true;
      auto pData = static_cast<const uint32_t *>(pByteCode->GetBufferPointer());
      size_t sizeInUint32 = pByteCode->GetBufferSize() / sizeof(uint32_t);
      finalResult.bytecode.assign(pData, pData + sizeInUint32);
    }
    return finalResult;
  }

  auto LoadBytecode(const std::filesystem::path &path)
      -> std::expected<std::vector<uint32_t>, EStatusCode> override {
    return std::unexpected(EStatusCode::InternalError);
  }
};
} // namespace avalon::shader_compiler

extern "C" AVALON_SHADER_COMPILER_API avalon::IPlugin *CreatePlugin() {
  return new avalon::shader_compiler::ShaderCompiler();
}

extern "C" AVALON_SHADER_COMPILER_API void
DestroyPlugin(avalon::IPlugin *plugin) {
  if (plugin) {
    plugin->Cleanup();
    delete plugin;
  }
}
