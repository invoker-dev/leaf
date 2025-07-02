#include "leafEngine.h"
#include <VkBootstrap.h>
#include <filesystem>
#include <fmt/base.h>
#include <fstream>
#include <leafInit.h>
#include <leafUtil.h>
#include <vulkan/vulkan_core.h>

namespace leafUtil {

void transitionImage(VkCommandBuffer cmd, VkImage image,
                     VkImageLayout currentLayout, VkImageLayout newLayout) {

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
        leafInit::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT);
  } else {
    imageBarrier.subresourceRange =
        leafInit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
  }
  imageBarrier.image = image;

  VkDependencyInfo dependencyInfo        = {};
  dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.imageMemoryBarrierCount = 1;
  dependencyInfo.pImageMemoryBarriers    = &imageBarrier;

  vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}

void copyImageToImage(VkCommandBuffer commandBuffer, VkImage src, VkImage dst,
                      VkExtent2D srcSize, VkExtent2D dstSize) {

  VkImageBlit2 blitRegion    = {};
  blitRegion.sType           = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
  blitRegion.srcOffsets[1].x = srcSize.width;
  blitRegion.srcOffsets[1].y = srcSize.height;
  blitRegion.srcOffsets[1].z = 1;

  blitRegion.dstOffsets[1].x = dstSize.width;
  blitRegion.dstOffsets[1].y = dstSize.height;
  blitRegion.dstOffsets[1].z = 1;

  blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.srcSubresource.baseArrayLayer = 0;
  blitRegion.srcSubresource.layerCount     = 1;
  blitRegion.srcSubresource.mipLevel       = 0;

  blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.dstSubresource.baseArrayLayer = 0;
  blitRegion.dstSubresource.layerCount     = 1;
  blitRegion.dstSubresource.mipLevel       = 0;

  VkBlitImageInfo2 blitInfo = {};
  blitInfo.sType            = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
  blitInfo.dstImage         = dst;
  blitInfo.dstImageLayout   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  blitInfo.srcImage         = src;
  blitInfo.srcImageLayout   = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  blitInfo.filter           = VK_FILTER_LINEAR;
  blitInfo.regionCount      = 1;
  blitInfo.pRegions         = &blitRegion;

  vkCmdBlitImage2(commandBuffer, &blitInfo);
}

VkShaderModule loadShaderModule(const std::string fileName, VkDevice device) {

  std::string   path = "build/shaders/" + fileName;
  std::ifstream file(path, std::ios::ate | std::ios::binary);

  fmt::println("Attempting to load shader from: {}",
               std::filesystem::absolute(path).string());

  if (!file.is_open()) {
    fmt::println("could not open file");
    return nullptr;
  }

  size_t fileSize = static_cast<size_t>(file.tellg());

  std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

  file.seekg(0);

  file.read((char*)buffer.data(), fileSize);

  file.close();

  VkShaderModuleCreateInfo info = {};
  info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize                 = buffer.size() * sizeof(uint32_t);
  info.pCode                    = buffer.data();

  VkShaderModule shaderModule;
  vkAssert(vkCreateShaderModule(device, &info, nullptr, &shaderModule));
  return shaderModule;
}
} // namespace leafUtil
