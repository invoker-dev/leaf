#include "VkBootstrapDispatch.h"
#include "leafUtil.h"
#include <cstdint>
#include <pipelineBuilder.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>

PipelineBuilder::PipelineBuilder(vkb::DispatchTable const& dispatch) {
  this->dispatch       = dispatch;
  shaderStages         = {};
  inputAssembly        = {};
  depthStencil         = {};
  rasterizer           = {};
  multiSampling        = {};
  pipelineLayout       = {};
  colorBlendAttachment = {};
  renderInfo           = {};

  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

  multiSampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

  renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderInfo.depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT;
  renderInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  shaderStages.clear();
}
VkPipeline PipelineBuilder::build() const {
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount  = 1;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable   = VK_FALSE;
  colorBlending.logicOp         = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments    = &colorBlendAttachment;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext      = &renderInfo;
  pipelineInfo.stageCount = (uint32_t)shaderStages.size();
  pipelineInfo.pStages    = shaderStages.data();
  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState      = &viewportState;
  pipelineInfo.pMultisampleState   = &multiSampling;
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDepthStencilState  = &depthStencil;
  pipelineInfo.layout              = pipelineLayout;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.renderPass          = VK_NULL_HANDLE;

  VkDynamicState                   state[]     = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicInfo = {};
  dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicInfo.pDynamicStates    = &state[0];
  dynamicInfo.dynamicStateCount = 2;

  pipelineInfo.pDynamicState = &dynamicInfo;

  VkPipeline newPipeline;
  vkAssert(dispatch.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo,
                                            nullptr, &newPipeline));
  return newPipeline;
}

PipelineBuilder& PipelineBuilder::setLayout(VkPipelineLayout layout) {
  pipelineLayout = layout;
  return *this;
}
PipelineBuilder& PipelineBuilder::setShaders(VkShaderModule vertexModule,
                                             VkShaderModule fragModule) {
  shaderStages.clear();

  VkPipelineShaderStageCreateInfo vertexInfo = {};
  vertexInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertexInfo.module = vertexModule;
  vertexInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo fragInfo = {};
  fragInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragInfo.module = fragModule;
  fragInfo.pName  = "main";

  shaderStages.push_back(vertexInfo);
  shaderStages.push_back(fragInfo);
  return *this;
}

PipelineBuilder&
PipelineBuilder::setInputTopology(VkPrimitiveTopology topology) {
  inputAssembly.topology               = topology;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  return *this;
}

PipelineBuilder& PipelineBuilder::setPolygonMode(VkPolygonMode mode) {
  rasterizer.polygonMode = mode;
  rasterizer.lineWidth   = 1.f;

  return *this;
}

PipelineBuilder& PipelineBuilder::setCullMode(VkCullModeFlags cullMode,
                                              VkFrontFace     frontFace) {
  rasterizer.cullMode  = cullMode;
  rasterizer.frontFace = frontFace;

  return *this;
}

PipelineBuilder& PipelineBuilder::disableMultiSampling() {
  multiSampling.sampleShadingEnable   = VK_FALSE;
  multiSampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
  multiSampling.minSampleShading      = 1.0f;
  multiSampling.pSampleMask           = nullptr;
  multiSampling.alphaToCoverageEnable = VK_FALSE;
  multiSampling.alphaToOneEnable      = VK_FALSE;

  return *this;
}

PipelineBuilder& PipelineBuilder::disableBlending() {
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  return *this;
}

PipelineBuilder& PipelineBuilder::setColorAttachmentFormat(VkFormat format) {
  colorAttachmentFormat              = format;
  renderInfo.colorAttachmentCount    = 1;
  renderInfo.pColorAttachmentFormats = &colorAttachmentFormat;

  return *this;
}

PipelineBuilder& PipelineBuilder::setDepthTest(bool mode) {
  if (mode) {
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;
    depthStencil.front                 = {};
    depthStencil.front.compareOp       = VK_COMPARE_OP_ALWAYS;
    depthStencil.back                  = {};
    depthStencil.back.compareOp        = VK_COMPARE_OP_ALWAYS;
    depthStencil.minDepthBounds        = 0.f;
    depthStencil.maxDepthBounds        = 1.f;
  } else {
    depthStencil.depthTestEnable       = VK_FALSE;
    depthStencil.depthWriteEnable      = VK_FALSE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_NEVER;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;
    depthStencil.front                 = {};
    depthStencil.back                  = {};
    depthStencil.minDepthBounds        = 0.f;
    depthStencil.maxDepthBounds        = 1.f;
  }

  return *this;
}
