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

constexpr int framesInFlight = 4;

class LeafEngine {
public:
  LeafEngine();
  ~LeafEngine();

  void draw();

private:
  vkb::Instance              instance;
  vkb::PhysicalDevice        physicalDevice;
  vkb::Device                device;
  vkb::Swapchain             swapchain;
  VkFormat                   swapchainImageFormat;
  vkb::InstanceDispatchTable instanceDispatchTable;
  vkb::DispatchTable         dispatch;
  VkExtent2D                 windowExtent;
  VkSurfaceKHR               surface;
  VkSurfaceFormatKHR         surfaceFormat;
  SDL_Window*                window;
  VmaAllocator               allocator;
  VulkanDestroyer            vulkanDestroyer;

  VkQueue  graphicsQueue;
  uint32_t graphicsQueueFamily;

  std::vector<VkImage>     swapchainImages;
  std::vector<VkImageView> swapchainImageViews;
  VkExtent2D               swapchainExtent;

  std::vector<VkFramebuffer> frameBuffers;
  uint32_t                   frameNumber;

  VkPipelineLayout pipelineLayout;
  VkPipeline       pipeline;

  AllocatedImage drawImage;
  VkExtent2D     drawExtent;

  VkDescriptorSet       drawImageDescriptors;
  VkDescriptorSetLayout drawImageDescriptorLayout;

  FrameData     frames[framesInFlight];
  FrameData&    getCurrentFrame() {
    return frames[frameNumber % framesInFlight];
  }

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void initSwapchain();
  void initCommands();
  void initSynchronization();

  void drawBackground(VkCommandBuffer cmd);
};
