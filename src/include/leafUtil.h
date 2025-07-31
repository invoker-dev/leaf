#pragma once
#include "VkBootstrapDispatch.h"
#include <VkBootstrap.h>

#include <fmt/base.h>
#include <fmt/core.h>
#include <vulkan/vk_enum_string_helper.h>
inline void VK_ASSERT(VkResult result) {
  if (result == VK_SUBOPTIMAL_KHR) {
    // fmt::println("suboptimal: {}", string_VkResult(result));
  } else if (result != VK_SUCCESS) {
    fmt::println("Detected Vulkan error: {}", string_VkResult(result));
    std::abort();
  }
}

inline void VK_IMGUI_ASSERT(VkResult result) {
  if (result == VK_SUBOPTIMAL_KHR) {
    // fmt::println("suboptimal: {}", string_VkResult(result));
  } else if (result != VK_SUCCESS) {
    fmt::println("Detected IMGUI::Vulkan error: {}", string_VkResult(result));
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
