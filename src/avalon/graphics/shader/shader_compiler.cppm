module;
#include <dxcapi.h>
#include <spirv_reflect.h>

export module avalon.shader:shader_compiler;
import avalon.core;
import :shader_blob_builder;
import :serialization;
import :utils;

using namespace avalon::rhi;

namespace {
using namespace avalon;
using namespace avalon::graphics;

void ExtractVertexInputs(SpvReflectShaderModule *pModule,
                         ReflectionData &outReflectionData) {
  uint32_t count = 0;
  spvReflectEnumerateInputVariables(pModule, &count, nullptr);
  std::vector<SpvReflectInterfaceVariable *> inputVars(count);
  spvReflectEnumerateInputVariables(pModule, &count, inputVars.data());

  for (auto *pVariable : inputVars) {
    if (pVariable->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
      continue;
    ShaderInputAttribute attr{.nameHash = StringId(pVariable->name),
                              .location = pVariable->location,
                              .format = ToEFormat(pVariable->format),
                              .semantic = ToESemantic(pVariable->semantic)};

    outReflectionData.inputAttributes.PushBack(attr);
  }
  std::sort(
      outReflectionData.inputAttributes.begin(),
      outReflectionData.inputAttributes.end(),
      [](const auto &a, const auto &b) { return a.location < b.location; });
}

void ExtractDescriptorBindings(SpvReflectShaderModule *module,
                               ReflectionData &reflectionData,
                               EShaderStage stage) {

  uint32_t count = 0;
  spvReflectEnumerateDescriptorSets(module, &count, nullptr);
  std::vector<SpvReflectDescriptorSet *> sets(count);
  spvReflectEnumerateDescriptorSets(module, &count, sets.data());

  for (auto *set : sets) {
    for (uint32_t i = 0; i < set->binding_count; i++) {
      SpvReflectDescriptorBinding *pSpvReflDescBinding = set->bindings[i];
      auto type = pSpvReflDescBinding->descriptor_type;
      auto descBinding = ShaderDescriptorBinding{
          .nameHash = StringId(
              pSpvReflDescBinding->name ? pSpvReflDescBinding->name : ""),
          .set = pSpvReflDescBinding->set,
          .bindingPoint = pSpvReflDescBinding->binding,
          .visibleStages = stage,
          .bufferSize = 0,
          .memberCount = 0,
          .memberOffset =
              static_cast<uint32_t>(reflectionData.bufferMembers.GetSize()),
          .count = pSpvReflDescBinding->count,
      };

      switch (type) {
      case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        descBinding.type = EDescriptorType::Sampler;
        descBinding.usage = EBufferUsage::None;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        descBinding.type = EDescriptorType::CombinedImageSampler;
        descBinding.usage = EBufferUsage::None;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        descBinding.type = EDescriptorType::SampledImage;
        descBinding.usage = EBufferUsage::None;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        descBinding.type = EDescriptorType::StorageImage;
        descBinding.usage = EBufferUsage::None;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        descBinding.type = EDescriptorType::UniformTexelBuffer;
        descBinding.usage = EBufferUsage::Uniform | EBufferUsage::TransferDst;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        descBinding.type = EDescriptorType::StorageTexelBuffer;
        descBinding.usage = EBufferUsage::Storage | EBufferUsage::TransferDst;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        descBinding.type = EDescriptorType::UniformBuffer;
        descBinding.usage = EBufferUsage::Uniform | EBufferUsage::TransferDst;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        descBinding.type = EDescriptorType::StorageBuffer;
        descBinding.usage = EBufferUsage::Storage | EBufferUsage::TransferDst;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        descBinding.type = EDescriptorType::UniformBuffer;
        descBinding.usage = EBufferUsage::Uniform | EBufferUsage::TransferDst;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        descBinding.type = EDescriptorType::StorageBuffer;
        descBinding.usage = EBufferUsage::Storage | EBufferUsage::TransferDst;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        descBinding.type = EDescriptorType::InputAttachment;
        descBinding.usage = EBufferUsage::None;
        break;
      case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        descBinding.type = EDescriptorType::AccelerationStructure;
        descBinding.usage = EBufferUsage::None;
        break;
      }

      if (pSpvReflDescBinding->descriptor_type ==
              SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
          pSpvReflDescBinding->descriptor_type ==
              SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
        descBinding.bufferSize = pSpvReflDescBinding->block.size;
        Debug("Buffer size: {}", pSpvReflDescBinding->block.size);
        descBinding.memberCount = pSpvReflDescBinding->block.member_count;

        for (uint32_t j = 0; j < pSpvReflDescBinding->block.member_count; j++) {
          const auto &spvReflMember = pSpvReflDescBinding->block.members[j];
          ShaderBufferMember bufferMember{
              .nameHash =
                  StringId(spvReflMember.name ? spvReflMember.name : ""),
              .offset = spvReflMember.offset,
              .size = spvReflMember.size,
              .bindingPoint = pSpvReflDescBinding->binding,
              .arrayStride = spvReflMember.array.stride,
              .format = SpvTypeToEFormat(spvReflMember.type_description),
              .defaultValueOffset = kNoDefaultValue};
          reflectionData.bufferMembers.PushBack(bufferMember);
        }
      }

      reflectionData.descBindings.PushBack(descBinding);
    }
  }
}

void ExtractPushConstantRanges(SpvReflectShaderModule *module,
                               ReflectionData &reflectionData,
                               EShaderStage stage) {
  uint32_t count = 0;
  spvReflectEnumeratePushConstantBlocks(module, &count, nullptr);
  std::vector<SpvReflectBlockVariable *> blocks(count);
  spvReflectEnumeratePushConstantBlocks(module, &count, blocks.data());

  for (auto *block : blocks) {
    ShaderPushConstant range{
        .visibleStages = stage,
        .offset = block->offset,
        .size = block->size,
    };
    reflectionData.pushConstantRanges.PushBack(range);
  }
}

auto ReflectShader(EShaderStage stage, const void *data, size_t size)
    -> ReflectionData {
  ReflectionData reflectionData;

  SpvReflectShaderModule module;
  SpvReflectResult result = spvReflectCreateShaderModule(size, data, &module);
  if (result != SPV_REFLECT_RESULT_SUCCESS) {
    avalon::Error("[shader_compiler]: failed to reflect shader.");
    return reflectionData;
  }

  if (stage == EShaderStage::Vertex)
    ExtractVertexInputs(&module, reflectionData);

  ExtractDescriptorBindings(&module, reflectionData, stage);

  ExtractPushConstantRanges(&module, reflectionData, stage);

  spvReflectDestroyShaderModule(&module);
  return reflectionData;
}

} // namespace

namespace avalon::graphics {

struct StageDesc {
  String entryPointName;
  EShaderStage shaderStage;
};

struct ShaderCompileDesc {
  BlobPtr sourceCode;
  Path filePath;
  Array<StageDesc> shaderStages;
  rhi::EShaderFeatureLevel level = rhi::EShaderFeatureLevel::Level_6_0;
};

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
  T **GetAddressOf() {
    if (m_ptr) {
      m_ptr->Release();
      m_ptr = nullptr;
    }
    return &m_ptr;
  }
  T *operator->() const { return m_ptr; }
  explicit operator bool() const { return m_ptr != nullptr; }

private:
  T *m_ptr;
};

class ShaderCompiler : public mem::AutoDestroyable<ShaderCompiler> {
public:
  bool Initialize() {
    HRESULT hr =
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.GetAddressOf()));
    if (FAILED(hr)) {
      avalon::Error("[shader compiler]: failed to create dxc utils.");
      return false;
    }

    hr = DxcCreateInstance(CLSID_DxcCompiler,
                           IID_PPV_ARGS(m_compiler.GetAddressOf()));
    if (FAILED(hr)) {
      avalon::Error("[shader compiler]: failed to create dxc compiler.");
      return false;
    }
    return true;
  }

  auto Compile(const ShaderCompileDesc &desc) -> BlobPtr {

    DxcPtr<IDxcBlobEncoding> pSourceBlob;
    m_utils->CreateBlobFromPinned(
        desc.sourceCode->GetData(),
        static_cast<uint32_t>(desc.sourceCode->GetSize()), DXC_CP_UTF8,
        pSourceBlob.GetAddressOf());

    DxcBuffer sourceBuffer{.Ptr = pSourceBlob->GetBufferPointer(),
                           .Size = pSourceBlob->GetBufferSize(),
                           .Encoding = DXC_CP_UTF8};

    ShaderBlobBuilder builder;
    bool isAnyStageCompiled = false;

    for (auto const &stageDesc : desc.shaderStages) {
      std::vector<LPCWSTR> wArgs;
      std::vector<std::wstring> localStrings;

      auto AddArg = [&](LPCWSTR &&arg) { wArgs.push_back(std::move(arg)); };

      auto AddDynamicArg = [&](std::wstring &&arg) {
        auto &ref = localStrings.emplace_back(std::move(arg));
        wArgs.push_back(ref.c_str());
      };

      if constexpr (platform::kIsLinux) {
        AddArg(L"-spirv");
        AddArg(L"-fspv-target-env=universal1.5");
      }
      AddArg(L"-fspv-reflect");
      AddArg(L"-D");
      AddArg(L"VK_LOCATION(n)=[[vk::location(n)]]");
      AddArg(L"-fvk-use-dx-layout");
      AddArg(L"-Fi");
      AddDynamicArg(Utf8ToWstring(desc.filePath.GetCStr()));

      auto wEntryPoint = Utf8ToWstring(stageDesc.entryPointName.GetData());
      auto wProfile = GetTargetProfile(stageDesc.shaderStage, desc.level);

      AddArg(L"-E");
      AddDynamicArg(std::move(wEntryPoint));
      AddArg(L"-T");
      AddDynamicArg(std::move(wProfile));

      DxcPtr<IDxcResult> pResult;

      auto hr = m_compiler->Compile(
          &sourceBuffer, reinterpret_cast<LPCWSTR *>(wArgs.data()),
          static_cast<uint32_t>(wArgs.size()), nullptr,
          IID_PPV_ARGS(pResult.GetAddressOf()));

      if (FAILED(hr)) {
        avalon::Error("[shader compiler]: failed to compile shader. Error: "
                      "internal error.");
        return nullptr;
      }

      DxcPtr<IDxcBlobUtf8> pErrors;
      pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()),
                         nullptr);
      if (pErrors && pErrors->GetStringLength() > 0) {
        Error("[shader compiler]: {}",
              reinterpret_cast<const char *>(pErrors->GetBufferPointer()));
      } else if (FAILED(hr)) {
        Error("[shader compiler]: failed to compile shader. Error: 0x{:X}.",
              (uint32_t)hr);
      }

      HRESULT status;
      pResult->GetStatus(&status);
      if (FAILED(status)) {
        Error("[shader compiler]: failed to retrieve binary output from DXC "
              "result. Error: internal.");
        return nullptr;
      }

      DxcPtr<IDxcBlob> pByteCode;
      pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pByteCode.GetAddressOf()),
                         nullptr);

      if (pByteCode) {
        isAnyStageCompiled = true;
        auto reflectionData =
            ReflectShader(stageDesc.shaderStage, pByteCode->GetBufferPointer(),
                          pByteCode->GetBufferSize());
        builder.AddStage(stageDesc.shaderStage,
                         StringId(stageDesc.entryPointName),
                         pByteCode->GetBufferPointer(),
                         pByteCode->GetBufferSize(), reflectionData);
      }
    }
    if (isAnyStageCompiled)
      return CreateBlob(builder.Build());
    return nullptr;
  }

private:
  DxcPtr<IDxcUtils> m_utils;
  DxcPtr<IDxcCompiler3> m_compiler;
};
} // namespace avalon::graphics
