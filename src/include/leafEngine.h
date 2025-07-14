#pragma once
#include <SDL3/SDL.h>
#include <VkBootstrap.h>
#include <fmt/core.h>
#include <vk_mem_alloc.h>

#include <leafStructs.h>
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

  AllocatedImage drawImage;
  VkExtent2D     drawExtent;

  VkPipelineLayout      pipelineLayout;
  VkPipeline            pipeline;
  VkDescriptorSet       drawImageDescriptors;
  VkDescriptorSetLayout drawImageDescriptorLayout;

  FrameData  frames[framesInFlight];
  FrameData& getCurrentFrame() { return frames[frameNumber % framesInFlight]; }

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void initSwapchain();
  void initCommands();
  void initSynchronization();

  void drawBackground(VkCommandBuffer cmd);
};
