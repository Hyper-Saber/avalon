module;
#include <debug/assert.hpp>
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
}

void ExtractPushConstant(SpvReflectShaderModule *pModule,
                         ReflectionData &reflectionData) {
  uint32_t blockCount = 0;
  SpvReflectResult result =
      spvReflectEnumeratePushConstantBlocks(pModule, &blockCount, nullptr);
  if (result != SPV_REFLECT_RESULT_SUCCESS || blockCount == 0)
    return;

  Array<SpvReflectBlockVariable *> pBlocks(blockCount);
  spvReflectEnumeratePushConstantBlocks(pModule, &blockCount,
                                        pBlocks.GetData());

  for (uint32_t i = 0; i < blockCount; i++) {
    const SpvReflectBlockVariable &rootBlock = *pBlocks[i];

    AVALON_ASSERT_MSG(
        rootBlock.size == sizeof(StandardPushConstant),
        "PushConstant block size is not equal to sizeof(StanardPushConstant)!");
    uint32_t slot = 0;
    auto customBlock = rootBlock.members[2];
    for (uint32_t j = 0; j < customBlock.member_count; j++) {
      auto member = customBlock.members[j];
      if (StringView(member.name) != "padding") {
        ShaderCustomPushConstantTextureSlot pushConstant;
        pushConstant.nameHash = StringId(member.name);
        pushConstant.textureSlot = slot++;

        reflectionData.pushConstantMembers.PushBack(pushConstant);
      }
    }
  }
}

void AddBufferMember(const SpvReflectBlockVariable &spvMember,
                     const String &fullName, uint32_t binding,
                     uint32_t absoluteOffset, ReflectionData &reflectionData,
                     ShaderDescriptorBinding &descBinding) {

  descBinding.memberCount++;
  descBinding.bufferSize += spvMember.size;

  ShaderBufferMember bufferMember;

  bufferMember.nameHash = StringId(fullName);
  bufferMember.offset = absoluteOffset;
  bufferMember.size = spvMember.size;
  bufferMember.bindingPoint = binding;
  bufferMember.arrayStride = spvMember.array.stride;
  bufferMember.format = SpvTypeToEFormat(spvMember.type_description);
  bufferMember.defaultValueOffset = kNoDefaultValue;
  reflectionData.bufferMembers.PushBack(bufferMember);
}

void ProcessStructMembers(const SpvReflectBlockVariable &block,
                          const String &prefix, uint32_t binding,
                          ReflectionData &reflectionData,
                          ShaderDescriptorBinding &descBinding,
                          uint32_t baseOffset) {

  auto isArray = [](const SpvReflectBlockVariable &var) {
    return var.array.dims_count > 0;
  };

  auto isStruct = [](const SpvReflectBlockVariable &var) {
    return var.member_count > 0;
  };

  for (uint32_t i = 0; i < block.member_count; i++) {
    const auto &member = block.members[i];
    if (member.name && StringView(member.name).Contains("padding"))
      continue;

    if (isArray(member)) {
      uint32_t elementStride = member.array.stride;
      for (uint32_t elementIdx = 0; elementIdx < member.array.dims[0];
           elementIdx++) {
        String arrayElementName =
            String::Format("{}.{}[{}]", prefix, member.name, elementIdx);
        uint32_t elementOffset =
            baseOffset + member.offset + (elementIdx * elementStride);

        if (isStruct(member)) {
          ProcessStructMembers(member, arrayElementName.GetData(), binding,
                               reflectionData, descBinding, elementOffset);
        } else {
          AddBufferMember(member, arrayElementName, binding, elementOffset,
                          reflectionData, descBinding);
        }
      }
    } else if (isStruct(member)) {
      uint32_t absoluteOffset = baseOffset + member.offset;
      ProcessStructMembers(member, prefix, binding, reflectionData, descBinding,
                           absoluteOffset);
    } else {
      uint32_t absoluteOffset = baseOffset + member.offset;
      AddBufferMember(member, String::Format("{}.{}", prefix, member.name),
                      binding, absoluteOffset, reflectionData, descBinding);
    }
  }
}

void ExtractDescriptorBindings(SpvReflectShaderModule *module,
                               ReflectionData &reflectionData,
                               EShaderStage stage) {

  uint32_t count = 0;
  spvReflectEnumerateDescriptorSets(module, &count, nullptr);
  std::vector<SpvReflectDescriptorSet *> sets(count);
  spvReflectEnumerateDescriptorSets(module, &count, sets.data());

  for (auto *set : sets) {
    if (set->set != kMaterialsBinding)
      continue;
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
      // --- 纯采样器 (无资源状态) ---
      case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        descBinding.type = EDescriptorType::Sampler;
        descBinding.usage = EResourceUsage::None;
        break;

      // --- (Image Based) ---
      case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        descBinding.type =
            (type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                ? EDescriptorType::CombinedImageSampler
                : EDescriptorType::SampledImage;
        descBinding.usage = EResourceUsage::ReadOnly;
        break;

      case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        descBinding.type = EDescriptorType::StorageImage;
        descBinding.usage =
            EResourceUsage::ReadWrite | EResourceUsage::TransferDst;
        break;

      case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        descBinding.type = EDescriptorType::InputAttachment;
        descBinding.usage = EResourceUsage::ReadOnly;
        break;

      // --- (Buffer Based) ---
      case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        descBinding.type = EDescriptorType::UniformBufferDynamic;
        descBinding.usage =
            EResourceUsage::UniformBuffer | EResourceUsage::TransferDst;
        break;

      case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        descBinding.type =
            (type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
                ? EDescriptorType::StorageBufferDynamic
                : EDescriptorType::StorageBuffer;
        descBinding.usage =
            EResourceUsage::ReadWrite | EResourceUsage::TransferDst;
        break;

      // --- Texel Buffer 类 (硬件是 Buffer，访问语义是 Image) ---
      case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        descBinding.type = EDescriptorType::UniformTexelBuffer;
        descBinding.usage =
            EResourceUsage::ReadOnly | EResourceUsage::TransferDst;
        break;

      case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        descBinding.type = EDescriptorType::StorageTexelBuffer;
        descBinding.usage =
            EResourceUsage::ReadWrite | EResourceUsage::TransferDst;
        break;

      // --- 光线追踪 ---
      case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        descBinding.type = EDescriptorType::AccelerationStructure;
        descBinding.usage = EResourceUsage::ReadOnly;
        break;
      }

      auto isBuffer = [](SpvReflectDescriptorType t) {
        return t == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
               t == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
               t == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
               t == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
      };

      if (isBuffer(pSpvReflDescBinding->descriptor_type)) {
        descBinding.memberCount = 0;

        const char *blockName = pSpvReflDescBinding->name;
        if (!blockName || strlen(blockName) == 0) {
          blockName = pSpvReflDescBinding->block.type_description->type_name;
        }

        ProcessStructMembers(pSpvReflDescBinding->block, blockName,
                             pSpvReflDescBinding->binding, reflectionData,
                             descBinding, 0);
      }

      reflectionData.descBindings.PushBack(descBinding);
    }
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

  if (stage == EShaderStage::Vertex) {
    ExtractVertexInputs(&module, reflectionData);
  }
  ExtractPushConstant(&module, reflectionData);

  ExtractDescriptorBindings(&module, reflectionData, stage);

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
        // AddArg(L"-fspv-extension=SPV_EXT_descriptor_indexing");
        AddArg(L"-fspv-target-env=universal1.5");
      }

      AddArg(L"-fspv-reflect");
      AddArg(L"-fspv-preserve-bindings");
      AddArg(L"-fvk-use-dx-layout");
      AddArg(L"-Zpc");

      Path shaderPath;
      auto res = vfs::GetVfs().GetAbsolute(
          Path(vfs::kShaderFolderVirtualPath) / StringView("."), shaderPath);

      AddArg(L"-I");
      AddDynamicArg(Utf8ToWstring(shaderPath.GetCStr()));

      auto wEntryPoint = Utf8ToWstring(stageDesc.entryPointName.GetData());
      auto wProfile = GetTargetProfile(stageDesc.shaderStage, desc.level);

      AddArg(L"-E");
      AddDynamicArg(std::move(wEntryPoint));
      AddArg(L"-T");
      AddDynamicArg(std::move(wProfile));
      AddDynamicArg(Utf8ToWstring(desc.filePath.GetCStr()));

      DxcPtr<IDxcResult> pResult;

      DxcPtr<IDxcIncludeHandler> pIncludeHandler;
      m_utils->CreateDefaultIncludeHandler(pIncludeHandler.GetAddressOf());

      auto hr = m_compiler->Compile(
          &sourceBuffer, reinterpret_cast<LPCWSTR *>(wArgs.data()),
          static_cast<uint32_t>(wArgs.size()), pIncludeHandler.Get(),
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
