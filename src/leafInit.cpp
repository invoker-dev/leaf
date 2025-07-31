#include <leafInit.h>
#include <vulkan/vulkan_core.h>

namespace leafInit {
VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectFlags) {

  VkImageSubresourceRange subResourceRange = {};
  subResourceRange.aspectMask              = aspectFlags;
  subResourceRange.baseMipLevel            = 0;
  subResourceRange.levelCount              = VK_REMAINING_MIP_LEVELS;
  subResourceRange.baseArrayLayer          = 0;
  subResourceRange.layerCount              = VK_REMAINING_ARRAY_LAYERS;

  return subResourceRange;
}

VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags,
                                  VkExtent3D extent) {

  VkImageCreateInfo info = {};
  info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.imageType         = VK_IMAGE_TYPE_2D;
  info.format            = format;
  info.extent            = extent;
  info.mipLevels         = 1;
  info.arrayLayers       = 1;
  info.samples           = VK_SAMPLE_COUNT_1_BIT;
  info.tiling            = VK_IMAGE_TILING_OPTIMAL;
  info.usage             = usageFlags;

  return info;
}

VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image,
                                          VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo info = {};
  info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
  info.image                 = image;
  info.format                = format;
  info.subresourceRange      = imageSubresourceRange(aspectFlags);

  return info;
}

VkDescriptorSetLayoutBinding
descriptorSetLayoutBinding(VkDescriptorType   type,
                           VkShaderStageFlags shaderStage) {

  VkDescriptorSetLayoutBinding binding = {.binding            = 0,
                                          .descriptorType     = type,
                                          .descriptorCount    = 1,
                                          .stageFlags         = shaderStage,
                                          .pImmutableSamplers = nullptr};

  return binding;
}

VkPipelineLayoutCreateInfo
pipelineLayoutCreateInfo(VkDescriptorSetLayout& layout, u32 count) {

  VkPipelineLayoutCreateInfo createInfo = {};
  createInfo.pSetLayouts                = &layout;
  createInfo.setLayoutCount             = count;
  createInfo.pushConstantRangeCount     = 0;

  // TODO: finish
}

VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags usage) {
  VkCommandBufferBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.flags = usage;
  return info;

}
} // namespace leafInit
