#pragma once
#include <SDL3/SDL.h>
#include <VkBootstrap.h>
#include <fmt/core.h>
#include <vk_mem_alloc.h>

#include <leafStructs.h>
#include <vulkan/vulkan_core.h>
#include <vulkanDestroyer.h>

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
  vkb::InstanceDispatchTable instanceDispatchTable;
  vkb::DispatchTable         dispatch;

  VkExtent2D         windowExtent;
  VkSurfaceKHR       surface;
  VkSurfaceFormatKHR surfaceFormat;
  SDL_Window*        window;

  VkQueue  graphicsQueue;
  uint32_t graphicsQueueFamily;

  vkb::Swapchain           swapchain;
  VkFormat                 swapchainImageFormat;
  std::vector<VkImage>     swapchainImages;
  std::vector<VkImageView> swapchainImageViews;
  VkExtent2D               swapchainExtent;

  VmaAllocator    allocator;
  VulkanDestroyer vulkanDestroyer;

  std::vector<VkFramebuffer> frameBuffers;
  uint32_t                   frameNumber;
  AllocatedImage             drawImage;
  VkExtent2D                 drawExtent;

  imguiContext imguiContext;

  FrameData  frames[framesInFlight];
  FrameData& getCurrentFrame() { return frames[frameNumber % framesInFlight]; }

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void initSwapchain();
  void initCommands();
  void initSynchronization();
  void initImGUI();

  void drawImGUI(VkCommandBuffer cmd, VkImageView targetImage);
  void drawBackground(VkCommandBuffer cmd);
};
