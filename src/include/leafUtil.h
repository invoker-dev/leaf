#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace leafUtil {
void transitionImage(VkCommandBuffer cmd, VkImage image,
                     VkImageLayout currentLayout, VkImageLayout newLayout);

void copyImageToImage (VkCommandBuffer commandBuffer, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);
}
