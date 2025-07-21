#include "VkBootstrapDispatch.h"
#include <VkBootstrap.h>
#include <vector>
#include <vulkan/vulkan_core.h>

class PipelineBuilder {
public:
  PipelineBuilder(vkb::DispatchTable const& dispatch);

  VkPipeline build() const;

  PipelineBuilder& setLayout(VkPipelineLayout layout);
  PipelineBuilder& setShaders(VkShaderModule vertex, VkShaderModule frag);
  PipelineBuilder& setInputTopology(VkPrimitiveTopology topology);
  PipelineBuilder& setPolygonMode(VkPolygonMode mode);
  PipelineBuilder& setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
  PipelineBuilder& disableMultiSampling();
  PipelineBuilder& disableBlending();
  PipelineBuilder& setColorAttachmentFormat(VkFormat format);
  PipelineBuilder& setDepthTest(bool mode);

private:
  vkb::DispatchTable                           dispatch;
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  VkPipelineInputAssemblyStateCreateInfo       inputAssembly;
  VkPipelineRasterizationStateCreateInfo       rasterizer;
  VkPipelineColorBlendAttachmentState          colorBlendAttachment;
  VkPipelineMultisampleStateCreateInfo         multiSampling;
  VkPipelineLayout                             pipelineLayout;
  VkPipelineDepthStencilStateCreateInfo        depthStencil;
  VkPipelineRenderingCreateInfo                renderInfo;
  VkFormat                                     colorAttachmentFormat;
};
