#include "VkBootstrapDispatch.h"
#include <VkBootstrap.h>
#include <vector>
#include <vulkan/vulkan_core.h>

class PipelineBuilder {
public:
  PipelineBuilder(vkb::DispatchTable dispatch);
  ~PipelineBuilder() {};

  VkPipeline build();

  void setLayout(VkPipelineLayout layout);
  void setShaders(VkShaderModule vertex, VkShaderModule frag);
  void setInputTopology(VkPrimitiveTopology topology);
  void setPolygonMode(VkPolygonMode mode);
  void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
  void disableMultiSampling();
  void disableBlending();
  void setColorAttachmentFormat(VkFormat format);
  void setDepthTest(bool mode);

private:
  vkb::DispatchTable dispatch;

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
