module;
#include <dxcapi.h>
#include <spirv_reflect.h>

export module avalon.shader:shader_compiler;
import avalon.core;
import :shader_blob_builder;
import :serialization;

using namespace avalon::rhi;

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

auto Utf8ToUtf16(const char *utf8Str) -> Array<char16_t> {
  Array<char16_t> result;
  if (!utf8Str)
    return result;

  const uint8_t *p = reinterpret_cast<const uint8_t *>(utf8Str);

  while (*p) {
    uint32_t cp = 0; // Unicode Code Point

    // 手动解码 UTF-8
    if ((*p & 0x80) == 0) { // 1-byte (ASCII)
      cp = *p++;
    } else if ((*p & 0xE0) == 0xC0) { // 2-bytes
      cp = (*p++ & 0x1F) << 6;
      cp |= (*p++ & 0x3F);
    } else if ((*p & 0xF0) == 0xE0) { // 3-bytes
      cp = (*p++ & 0x0F) << 12;
      cp |= (*p++ & 0x3F) << 6;
      cp |= (*p++ & 0x3F);
    } else if ((*p & 0xF8) == 0xF0) { // 4-bytes
      cp = (*p++ & 0x07) << 18;
      cp |= (*p++ & 0x3F) << 12;
      cp |= (*p++ & 0x3F) << 6;
      cp |= (*p++ & 0x3F);
    } else {
      p++; // 非法序列，跳过
      continue;
    }

    // 编码为 UTF-16 (char16_t)
    if (cp <= 0xFFFF) {
      result.PushBack(static_cast<char16_t>(cp));
    } else {
      // 处理 Surrogate Pairs (超出 16 位范围的字符，如 Emoji)
      cp -= 0x10000;
      result.PushBack(static_cast<char16_t>((cp >> 10) + 0xD800));
      result.PushBack(static_cast<char16_t>((cp & 0x3FF) + 0xDC00));
    }
  }

  result.PushBack(u'\0'); // 确保以 null 结尾，DXC 接口需要
  return result;
}

auto Utf8ToWstring(const char *utf8Str) -> std::wstring {
  if (!utf8Str)
    return L"";

  size_t len = std::mbstowcs(nullptr, utf8Str, 0);
  if (len == (size_t)-1)
    return L"";
  std::wstring result(len, L'\0');
  std::mbstowcs(&result[0], utf8Str, len);
  return result;
}

static auto GetTargetProfile(EShaderStage stage, EShaderFeatureLevel level)
    -> std::wstring {
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
    return L"cs_" + version;
  default:
    return L"lib_" + version;
  }
}

EFormat ToEFormat(const SpvReflectFormat format) {
  switch (format) {
  case SPV_REFLECT_FORMAT_UNDEFINED:
    return EFormat::Undefined;
  case SPV_REFLECT_FORMAT_R16_UINT:
    return EFormat::R16_Uint;
  case SPV_REFLECT_FORMAT_R16_SINT:
    return EFormat::R16_Int;
  case SPV_REFLECT_FORMAT_R16_SFLOAT:
    return EFormat::R16_Float;
  case SPV_REFLECT_FORMAT_R16G16_UINT:
    return EFormat::R16G16_Uint2;
  case SPV_REFLECT_FORMAT_R16G16_SINT:
    return EFormat::R16G16_Int2;
  case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
    return EFormat::R16G16_Float2;
  case SPV_REFLECT_FORMAT_R16G16B16_UINT:
    return EFormat::R16G16B16_Uint3;
  case SPV_REFLECT_FORMAT_R16G16B16_SINT:
    return EFormat::R16G16B16_Int3;
  case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
    return EFormat::R16G16B16_Float3;
  case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
    return EFormat::R16G16B16A16_Uint4;
  case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
    return EFormat::R16G16B16A16_Int4;
  case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
    return EFormat::R16G16B16A16_Float4;
  case SPV_REFLECT_FORMAT_R32_UINT:
    return EFormat::R32_Uint;
  case SPV_REFLECT_FORMAT_R32_SINT:
    return EFormat::R32_Int;
  case SPV_REFLECT_FORMAT_R32_SFLOAT:
    return EFormat::R32_Float;
  case SPV_REFLECT_FORMAT_R32G32_UINT:
    return EFormat::R32G32_Uint2;
  case SPV_REFLECT_FORMAT_R32G32_SINT:
    return EFormat::R32G32_Int2;
  case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
    return EFormat::R32G32_Float2;
  case SPV_REFLECT_FORMAT_R32G32B32_UINT:
    return EFormat::R32G32B32_Uint3;
  case SPV_REFLECT_FORMAT_R32G32B32_SINT:
    return EFormat::R32G32B32_Int3;
  case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
    return EFormat::R32G32B32_Float3;
  case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
    return EFormat::R32G32B32A32_Uint4;
  case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
    return EFormat::R32G32B32A32_Int4;
  case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
    return EFormat::R32G32B32A32_Float4;
  case SPV_REFLECT_FORMAT_R64_UINT:
    return EFormat::R64_Uint;
  case SPV_REFLECT_FORMAT_R64_SINT:
    return EFormat::R64_Int;
  case SPV_REFLECT_FORMAT_R64_SFLOAT:
    return EFormat::R64_Float;
  case SPV_REFLECT_FORMAT_R64G64_UINT:
    return EFormat::R64G64_Uint2;
  case SPV_REFLECT_FORMAT_R64G64_SINT:
    return EFormat::R64G64_Int2;
  case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
    return EFormat::R64G64_Float2;
  case SPV_REFLECT_FORMAT_R64G64B64_UINT:
    return EFormat::R64G64B64_Uint3;
  case SPV_REFLECT_FORMAT_R64G64B64_SINT:
    return EFormat::R64G64B64_Int3;
  case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
    return EFormat::R64G64B64_Float3;
  case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:
    return EFormat::R64G64B64A64_Uint4;
  case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:
    return EFormat::R64G64B64A64_Int4;
  case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
    return EFormat::R64G64B64A64_Float4;
  }
}

EFormat SpvTypeToEFormat(const SpvReflectTypeDescription *type) {
  if (!type)
    return EFormat::Undefined;
  const auto &numeric = type->traits.numeric;
  const uint32_t componentCount = numeric.vector.component_count;
  const uint32_t width = numeric.scalar.width;

  if (type->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
    if (width == 16) {
      switch (componentCount) {
      case 1:
        return EFormat::R16_Float;
      case 2:
        return EFormat::R16G16_Float2;
      case 3:
        return EFormat::R16G16B16_Float3;
      case 4:
        return EFormat::R16G16B16A16_Float4;
      };
    } else if (width == 32)
      switch (componentCount) {
      case 1:
        return EFormat::R32_Float;
      case 2:
        return EFormat::R32G32_Float2;
      case 3:
        return EFormat::R32G32B32_Float3;
      case 4:
        return EFormat::R32G32B32A32_Float4;
      }
    else if (width == 64) {
      switch (componentCount) {
      case 1:
        return EFormat::R64_Float;
      case 2:
        return EFormat::R64G64_Float2;
      case 3:
        return EFormat::R64G64B64_Float3;
      case 4:
        return EFormat::R64G64B64A64_Float4;
      }
    }
  } else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
    bool is_signed = type->traits.numeric.scalar.signedness != 0;
    if (width == 16) {
      switch (componentCount) {
      case 1:
        return is_signed ? EFormat::R16_Int : EFormat::R16_Uint;
      case 2:
        return is_signed ? EFormat::R16G16_Int2 : EFormat::R16G16_Uint2;
      case 3:
        return is_signed ? EFormat::R16G16B16_Int3 : EFormat::R16G16B16_Uint3;
      case 4:
        return is_signed ? EFormat::R16G16B16A16_Int4
                         : EFormat::R16G16B16A16_Uint4;
      };
    } else if (width == 32) {
      switch (componentCount) {
      case 1:
        return is_signed ? EFormat::R32_Int : EFormat::R32_Uint;
      case 2:
        return is_signed ? EFormat::R32G32_Int2 : EFormat::R32G32_Uint2;
      case 3:
        return is_signed ? EFormat::R32G32B32_Int3 : EFormat::R32G32B32_Uint3;
      case 4:
        return is_signed ? EFormat::R32G32B32A32_Int4
                         : EFormat::R32G32B32A32_Uint4;
      }
    } else if (width == 64) {
      switch (componentCount) {
      case 1:
        return is_signed ? EFormat::R64_Int : EFormat::R64_Uint;
      case 2:
        return is_signed ? EFormat::R64G64_Int2 : EFormat::R64G64_Uint2;
      case 3:
        return is_signed ? EFormat::R64G64B64_Int3 : EFormat::R64G64B64_Uint3;
      case 4:
        return is_signed ? EFormat::R64G64B64A64_Int4
                         : EFormat::R64G64B64A64_Uint4;
      }
    }
  } else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
    return EFormat::Undefined;
  }

  return EFormat::Undefined;
}

void ExtractVertexInputs(SpvReflectShaderModule *pModule,
                         ReflectionData &outReflectionData) {
  uint32_t count = 0;
  spvReflectEnumerateInputVariables(pModule, &count, nullptr);
  std::vector<SpvReflectInterfaceVariable *> inputVars(count);
  spvReflectEnumerateInputVariables(pModule, &count, inputVars.data());

  for (auto *pVariable : inputVars) {
    if (pVariable->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
      continue;
    ShaderInputAttribute attr{
        .nameHash = StringId(pVariable->name),
        .location = pVariable->location,
        .format = ToEFormat(pVariable->format),
    };

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

      if constexpr (avalon::kIsLinux) {
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
  auto ReflectShader(EShaderStage stage, const void *data, size_t size)
      -> ReflectionData {
    ReflectionData reflectionData;

    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(size, data, &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
      avalon::Error("[shader_compiler]: failed to reflect shader.");
      return reflectionData;
    }

    ExtractVertexInputs(&module, reflectionData);

    ExtractDescriptorBindings(&module, reflectionData, stage);

    ExtractPushConstantRanges(&module, reflectionData, stage);

    spvReflectDestroyShaderModule(&module);
    return reflectionData;
  }
  DxcPtr<IDxcUtils> m_utils;
  DxcPtr<IDxcCompiler3> m_compiler;
};
} // namespace avalon::graphics
