#include <vkutil.h>
#include <vulkan/vulkan_core.h>
#include <VkBootstrap.h>

void vkutil::transitionImage(VkCommandBuffer cmd, VkImage image,
                             VkImageLayout currentLayout,
                             VkImageLayout newLayout) {
  VkImageMemoryBarrier2 imageBarrier = {};
  imageBarrier.sType                 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  imageBarrier.srcAccessMask         = VK_ACCESS_2_MEMORY_WRITE_BIT;
  imageBarrier.dstAccessMask =
      VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
  imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

  imageBarrier.oldLayout = currentLayout;
  imageBarrier.newLayout = newLayout;
  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
    imageBarrier.subresourceRange =
        vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT);
  } else {
    imageBarrier.subresourceRange =
        vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
  }
  imageBarrier.image = image;

  VkDependencyInfo dependencyInfo        = {};
  dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.imageMemoryBarrierCount = 1;
  dependencyInfo.pImageMemoryBarriers    = &imageBarrier;

  vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}

VkImageSubresourceRange
vkinit::imageSubresourceRange(VkImageAspectFlags aspectMask) {

  VkImageSubresourceRange subResourceRange = {};
  subResourceRange.aspectMask              = aspectMask;
  subResourceRange.baseMipLevel            = 0;
  subResourceRange.levelCount              = VK_REMAINING_MIP_LEVELS;
  subResourceRange.baseArrayLayer          = 0;
  subResourceRange.layerCount              = VK_REMAINING_ARRAY_LAYERS;

  return subResourceRange;
}
