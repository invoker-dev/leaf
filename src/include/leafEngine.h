#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <VkBootstrap.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct Init {
  vkb::Instance       instance;
  vkb::PhysicalDevice physicalDevice;
  vkb::Device         device;
  VkExtent2D          windowExtent;
  VkSurfaceKHR        surface;
  vkb::Swapchain      swapchain;
  SDL_Window         *window;
};

struct RenderData {

  VkQueue graphicsQueue;
  VkQueue presentQueue;

  std::vector<VkImage>       swapchainImages;
  std::vector<VkImageView>   swapchainImageViews;
  std::vector<VkFramebuffer> frameBuffers;

  VkPipelineLayout pipelineLayout;
  VkPipeline       pipeline;

  VkCommandPool                commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
};

class LeafEngine {
public:
  LeafEngine();
  ~LeafEngine();

private:
  Init       init;
  RenderData renderData;

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void createSwapchain();
  void createGraphicsPipeline();
};
