
#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace leafInit {
VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);
VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

}
