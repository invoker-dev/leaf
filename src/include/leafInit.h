#pragma once

#include <VkBootstrap.h>
#include <vulkan/vulkan_core.h>
#include <types.h>

namespace leafInit {
VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);
VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags,
                                  VkExtent3D extent);
VkImageViewCreateInfo        imageViewCreateInfo(VkFormat format, VkImage image,
                                                 VkImageAspectFlags aspectFlags);
VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type,
                                                        VkShaderStageFlags shaderStage);

VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(VkDescriptorSetLayout& layout, u32 count);

} // namespace leafInit
