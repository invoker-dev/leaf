#pragma once

#include<VkBootstrap.h>

namespace leafInit {
VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);
VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

}
