module;
#include <vulkan/vulkan.h>
export module avalon.rhi.vulkan:pipeline_builder;

import avalon.rhi;
import :utils;
import avalon.core;

namespace avalon::rhi {
class PipelineBuilder : public NonCopyable {
public:
  PipelineBuilder() {
    m_inputAssemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    };

    m_rasterizationInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    };

    m_multisampleStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    };

    m_viewportStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    m_depthStencilStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthBoundsTestEnable = VK_FALSE,
    };
    m_dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.GetSize()),
        .pDynamicStates = m_dynamicStates.GetData(),
    };
  }

  PipelineBuilder &LoadStates(const PipelineCreateInfo &info) {
    m_inputAssemblyInfo.topology =
        ToVkPrimitiveTopology(info.inputAssemblyState.topology);
    m_rasterizationInfo.polygonMode =
        ToVkPolygonMode(info.rasterizationState.polygonMode);
    m_rasterizationInfo.cullMode =
        ToVkCullMode(info.rasterizationState.cullMode);
    m_rasterizationInfo.frontFace =
        info.rasterizationState.frontFace == EFrontFace::CounterClockwise
            ? VK_FRONT_FACE_COUNTER_CLOCKWISE
            : VK_FRONT_FACE_CLOCKWISE;
    m_rasterizationInfo.lineWidth = info.rasterizationState.lineWidth;
    m_multisampleStateInfo.rasterizationSamples =
        ToVkSampleCount(info.multisampleState.sampleCount);
    m_depthStencilStateInfo.depthTestEnable =
        info.depthStencilState.isDepthTestEnable ? VK_TRUE : VK_FALSE;
    m_depthStencilStateInfo.depthWriteEnable =
        info.depthStencilState.isDepthWriteEnable ? VK_TRUE : VK_FALSE;
    m_depthStencilStateInfo.depthCompareOp =
        ToVkCompareOp(info.depthStencilState.depthCompareOp);
    m_depthStencilStateInfo.stencilTestEnable =
        info.depthStencilState.isStencilTestEnable;

    for (const auto &blendState : info.colorBlendStates) {
      m_colorBlendAttachmentStates.PushBack({
          .blendEnable = blendState.isEnable ? VK_TRUE : VK_FALSE,
          .srcColorBlendFactor = ToVkBlendFactor(blendState.srcColorFactor),
          .dstColorBlendFactor = ToVkBlendFactor(blendState.dstColorFactor),
          .colorBlendOp = ToVkBlendOp(blendState.colorOp),
          .srcAlphaBlendFactor = ToVkBlendFactor(blendState.srcAlphaFactor),
          .dstAlphaBlendFactor = ToVkBlendFactor(blendState.dstAlphaFactor),
          .alphaBlendOp = ToVkBlendOp(blendState.alphaOp),
          .colorWriteMask = ToVkColorComponentFlags(blendState.writeMask),
      });
    }

    m_colorBlendStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount =
            static_cast<uint32_t>(m_colorBlendAttachmentStates.GetSize()),
        .pAttachments = m_colorBlendAttachmentStates.GetData(),
    };

    return *this;
  }

  PipelineBuilder &SetVertexInput(Span<const VertexBinding> bindings,
                                  Span<const VertexInputAttribute> attributes) {
    m_bindingDescriptions.Clear();
    m_attributeDescriptions.Clear();

    for (const auto &binding : bindings) {
      m_bindingDescriptions.PushBack({
          .binding = binding.binding,
          .stride = binding.stride,
          .inputRate = binding.isInstanceData ? VK_VERTEX_INPUT_RATE_INSTANCE
                                              : VK_VERTEX_INPUT_RATE_VERTEX,
      });
      if constexpr (debug::kIsDebug) {
        Debug("[Vulkan]: Vertex input binding description: \n"
              "---------------------------------------------\n"
              "binding: {}\n stride: {}\n inputRate: {}\n"
              "---------------------------------------------",
              binding.binding, binding.stride, binding.isInstanceData);
      }
    }

    for (const auto &attr : attributes) {
      VkVertexInputAttributeDescription attrDesc{
          .location = attr.location,
          .binding = attr.binding,
          .format = ToVkFormat(attr.format),
          .offset = attr.offset,
      };

      m_attributeDescriptions.PushBack(attrDesc);

      if constexpr (debug::kIsDebug) {
        Debug("[Vulkan]: Vertex Input attribute description: \n"
              "---------------------------------------------\n"
              " location : {}\n binding : {}\n format: {}\n offset: {}\n"
              "---------------------------------------------",
              attrDesc.location, attrDesc.binding, ToView(attr.format),
              attrDesc.offset);
      }
    }

    m_vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount =
            static_cast<uint32_t>(m_bindingDescriptions.GetSize()),
        .pVertexBindingDescriptions = m_bindingDescriptions.GetData(),
        .vertexAttributeDescriptionCount =
            static_cast<uint32_t>(m_attributeDescriptions.GetSize()),
        .pVertexAttributeDescriptions = m_attributeDescriptions.GetData(),
    };

    return *this;
  }

  PipelineBuilder &AddShaderStage(EShaderStage stage, const String &entryName,
                                  VkShaderModule module) {
    m_entryPointNames.PushBack(entryName);

    VkPipelineShaderStageCreateInfo stageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = ToVkShaderStageBits(stage),
        .module = module,
        .pName = m_entryPointNames.GetBack().GetData(),
    };

    m_shaderStageCreateInfos.PushBack(stageCreateInfo);
    return *this;
  }

  PipelineBuilder &SetRenderPass(VkRenderPass renderPass,
                                 uint32_t subPass = 0) {
    m_renderPass = renderPass;
    m_subpass = subPass;
    return *this;
  }

  auto Build(VkDevice device, VkPipelineLayout layout) -> VkPipeline {

    VkGraphicsPipelineCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(m_shaderStageCreateInfos.GetSize()),
        .pStages = m_shaderStageCreateInfos.GetData(),
        .pVertexInputState = &m_vertexInputInfo,
        .pInputAssemblyState = &m_inputAssemblyInfo,
        .pViewportState = &m_viewportStateInfo,
        .pRasterizationState = &m_rasterizationInfo,
        .pMultisampleState = &m_multisampleStateInfo,
        .pDepthStencilState = &m_depthStencilStateInfo,
        .pColorBlendState = &m_colorBlendStateInfo,
        .pDynamicState = &m_dynamicStateInfo,
        .layout = layout,
        .renderPass = m_renderPass,
        .subpass = m_subpass,
    };

    VkPipeline pipeline{VK_NULL_HANDLE};
    auto vkRes = vkCreateGraphicsPipelines(device, nullptr, 1, &createInfo,
                                           nullptr, &pipeline);

    InternalClear(device);

    if (vkRes != VK_SUCCESS) {
      Error("[VK_RHI]: failed to create pipeline! Error Code: {}",
            ToView(vkRes));
    }

    return pipeline;
  }

private:
  VkPipelineVertexInputStateCreateInfo m_vertexInputInfo{};
  VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo{};
  VkPipelineViewportStateCreateInfo m_viewportStateInfo{};
  VkPipelineRasterizationStateCreateInfo m_rasterizationInfo{};
  VkPipelineMultisampleStateCreateInfo m_multisampleStateInfo{};
  VkPipelineColorBlendStateCreateInfo m_colorBlendStateInfo{};
  VkPipelineDepthStencilStateCreateInfo m_depthStencilStateInfo{};
  VkPipelineDynamicStateCreateInfo m_dynamicStateInfo{};
  Array<VkPipelineShaderStageCreateInfo> m_shaderStageCreateInfos;

  Array<VkDynamicState> m_dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                           VK_DYNAMIC_STATE_SCISSOR};
  Array<VkVertexInputBindingDescription> m_bindingDescriptions;
  Array<VkVertexInputAttributeDescription> m_attributeDescriptions;
  Array<VkPipelineColorBlendAttachmentState> m_colorBlendAttachmentStates;

  Array<String> m_entryPointNames;

  VkRenderPass m_renderPass{VK_NULL_HANDLE};
  uint32_t m_subpass = 0;

  void InternalClear(VkDevice device) { m_shaderStageCreateInfos.Clear(); }
};
} // namespace avalon::rhi
