module;
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan:pipeline_builder;

import avalon.rhi;
import :utils;
import :types;
import avalon.core;

namespace avalon::rhi {

class PipelineBuilder final : public NonCopyable {
public:
  PipelineBuilder() {
    m_inputAssemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    };

    m_rasterizationInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0f,
    };

    m_multisampleStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    m_viewportStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    m_depthStencilStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };

    m_dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.GetSize()),
        .pDynamicStates = m_dynamicStates.GetData(),
    };

    m_renderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    };
  }

  PipelineBuilder &LoadStates(const PipelineCreateInfo &info) {
    m_inputAssemblyInfo.topology =
        ToVkPrimitiveTopology(info.inputAssemblyState.topology);
    m_inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    m_rasterizationInfo.polygonMode =
        ToVkPolygonMode(info.rasterizationState.polygonMode);
    m_rasterizationInfo.cullMode =
        ToVkCullMode(info.rasterizationState.cullMode);
    m_rasterizationInfo.frontFace =
        (info.rasterizationState.frontFace == EFrontFace::CounterClockwise)
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
        info.depthStencilState.isStencilTestEnable ? VK_TRUE : VK_FALSE;

    m_colorBlendAttachmentStates.Clear();
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

    SetDynamicRenderingInfo(info.renderingInfo);

    return *this;
  }

  PipelineBuilder &SetVertexInput(Span<const VertexBinding> bindings,
                                  Span<const VertexInputAttribute> attributes) {
    m_bindingDescriptions.Clear();
    m_attributeDescriptions.Clear();

    for (const auto &b : bindings) {
      m_bindingDescriptions.PushBack({
          .binding = b.binding,
          .stride = b.stride,
          .inputRate = b.isInstanceData ? VK_VERTEX_INPUT_RATE_INSTANCE
                                        : VK_VERTEX_INPUT_RATE_VERTEX,
      });

      Debug("Vertex binding: \n binding: {}, stride: {}, inputRate: {}",
            b.binding, b.stride, b.isInstanceData);
    }

    for (const auto &a : attributes) {
      m_attributeDescriptions.PushBack({
          .location = a.location,
          .binding = a.binding,
          .format = ToVkFormat(a.format),
          .offset = a.offset,
      });

      Debug("Vertex attribute: \n location: {}, binding: {}, format: {}, "
            "offset: {}",
            a.location, a.binding, ToView(a.format), a.offset);
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

    m_shaderStageCreateInfos.PushBack({
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = ToVkShaderStageBit(stage),
        .module = module,
        .pName = m_entryPointNames.GetBack().GetData(),
    });
    return *this;
  }

  auto Build(VkDevice device, VkPipelineLayout layout) -> VkPipeline {
    VkGraphicsPipelineCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &m_renderingCreateInfo,
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
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
    };

    VkPipeline pipeline{VK_NULL_HANDLE};
    VkResult result = vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);

    if (result != VK_SUCCESS) {
      Error("[Vulkan RHI]: Failed to create graphics pipeline! Result: {}",
            (int)result);
    }

    return pipeline;
  }

private:
  void SetDynamicRenderingInfo(const PipelineRenderingInfo &info) {
    m_colorFormats.Clear();
    for (auto f : info.colorAttachmentFormats) {
      m_colorFormats.PushBack(ToVkFormat(f));
    }

    m_renderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = info.viewMask,
        .colorAttachmentCount = static_cast<uint32_t>(m_colorFormats.GetSize()),
        .pColorAttachmentFormats = m_colorFormats.GetData(),
        .depthAttachmentFormat = ToVkFormat(info.depthAttachmentFormat),
        .stencilAttachmentFormat = ToVkFormat(info.stencilAttachmentFormat),
    };
  }

  VkPipelineVertexInputStateCreateInfo m_vertexInputInfo{};
  VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo{};
  VkPipelineViewportStateCreateInfo m_viewportStateInfo{};
  VkPipelineRasterizationStateCreateInfo m_rasterizationInfo{};
  VkPipelineMultisampleStateCreateInfo m_multisampleStateInfo{};
  VkPipelineColorBlendStateCreateInfo m_colorBlendStateInfo{};
  VkPipelineDepthStencilStateCreateInfo m_depthStencilStateInfo{};
  VkPipelineDynamicStateCreateInfo m_dynamicStateInfo{};

  VkPipelineRenderingCreateInfo m_renderingCreateInfo{};
  Array<VkFormat> m_colorFormats;

  Array<VkPipelineShaderStageCreateInfo> m_shaderStageCreateInfos;
  Array<VkDynamicState> m_dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                           VK_DYNAMIC_STATE_SCISSOR};
  Array<VkVertexInputBindingDescription> m_bindingDescriptions;
  Array<VkVertexInputAttributeDescription> m_attributeDescriptions;
  Array<VkPipelineColorBlendAttachmentState> m_colorBlendAttachmentStates;
  Array<String> m_entryPointNames;
};

} // namespace avalon::rhi
