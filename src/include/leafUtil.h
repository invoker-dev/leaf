#pragma once
#include "VkBootstrapDispatch.h"
#include <VkBootstrap.h>

#include <fmt/core.h>
#include <vulkan/vk_enum_string_helper.h>
inline void vkAssert(VkResult result) {
  if (result == VK_SUBOPTIMAL_KHR) {
    // do nothing, for now. Should recreate swapchain
    // only a problem on X11
  } else if (result != VK_SUCCESS) {
    fmt::print("Detected Vulkan error: {}\n", string_VkResult(result));
    std::abort();
  }
}

namespace leafUtil {
void transitionImage(VkCommandBuffer cmd, VkImage image,
                     VkImageLayout currentLayout, VkImageLayout newLayout);

void copyImageToImage(VkCommandBuffer commandBuffer, VkImage src, VkImage dst,
                      VkExtent2D srcSize, VkExtent2D dstSize);

VkShaderModule loadShaderModule(const std::string& name,
                                vkb::DispatchTable dispatch);
} // namespace leafUtil
