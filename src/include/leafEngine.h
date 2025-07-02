#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <VkBootstrap.h>
#include <fmt/base.h>
#include <leafStructs.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

inline void vkAssert(VkResult result) {
  if (result == VK_SUBOPTIMAL_KHR) {
    // do nothing, for now. Should recreate swapchain
    // only a problem on X11
  } else if (result != VK_SUCCESS) {
    fmt::print("Detected Vulkan error: {}\n", string_VkResult(result));
    std::abort();
  }
}

constexpr bool useValidationLayers = true;

constexpr int framesInFlight = 2;

class LeafEngine {
public:
  LeafEngine();
  ~LeafEngine();

  void draw();

private:
  VulkanContext context;
  RenderData    renderData;
  FrameData     frames[framesInFlight];
  FrameData&    getCurrentFrame() {
    return frames[renderData.frameNumber % framesInFlight];
  }

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void initSwapchain();
  void initCommands();
  void initSynchronization();
  void initDescriptors();
  void initPipelines();

  void drawBackground(VkCommandBuffer cmd);
};
