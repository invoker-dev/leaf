#pragma once
#include "VkBootstrapDispatch.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <VkBootstrap.h>
#include <fmt/base.h>
#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

inline void vkAssert(VkResult result) {
  if (result != VK_SUCCESS) {
    fmt::print("Detected Vulkan error: {}\n", string_VkResult(result));
    std::abort();
  }
}

constexpr bool useValidationLayers = true;

constexpr int framesInFlight = 2;

struct VulkanContext {
  vkb::Instance              instance;
  vkb::PhysicalDevice        physicalDevice;
  vkb::Device                device;
  vkb::Swapchain             swapchain;
  VkFormat                   swapchainImageFormat;
  vkb::InstanceDispatchTable instanceDispatchTable;
  vkb::DispatchTable         dispatchTable;
  VkExtent2D                 windowExtent;
  VkSurfaceKHR               surface;
  VkSurfaceFormatKHR         surfaceFormat;
  SDL_Window*                window;
};

struct RenderData {

  VkQueue  graphicsQueue;
  uint32_t graphicsQueueFamily;
  // VkQueue presentQueue;

  std::vector<VkImage>       swapchainImages;
  std::vector<VkImageView>   swapchainImageViews;
  std::vector<VkFramebuffer> frameBuffers;
  uint32_t                   frameNumber;

  VkPipelineLayout pipelineLayout;
  VkPipeline       pipeline;
};

struct FrameData {

  VkCommandPool   commandPool;
  VkCommandBuffer mainCommandBuffer;
  VkSemaphore     swapchainSemaphore, renderSemaphore;
  VkFence         renderFence;
  // DeletionQueue   deletionQueue;
};

struct AllocatedImage {
  VkImage     image;
  VkImageView imageView;
  // VmaAllocation allocation;
  VkExtent3D imageExtent;
  VkFormat   imageFormat;
};

class LeafEngine {
public:
  LeafEngine();
  ~LeafEngine();

  void draw();

private:
  VulkanContext context;
  RenderData    renderData;
  FrameData     frameData;
  FrameData     frames[framesInFlight];
  FrameData&    getCurrentFrame() { return frames[renderData.frameNumber]; }

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void createSwapchain();
  void initCommands();
  void initSynchronization();
  // void recordCommandBuffer(uint32_t imageIndex);
};
