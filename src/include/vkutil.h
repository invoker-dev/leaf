#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace vkutil {
void transitionImage(VkCommandBuffer cmd, VkImage image,
                     VkImageLayout currentLayout, VkImageLayout newLayout);

} // namespace vkutil

namespace vkinit {
VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);

}
