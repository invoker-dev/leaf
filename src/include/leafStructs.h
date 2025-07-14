#pragma once
#include <SDL3/SDL.h>
#include <VkBootstrap.h>
#include <span>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include<optional>

struct AllocatedImage {
  VkImage       image;
  VkImageView   imageView;
  VmaAllocation allocation;
  VkExtent3D    imageExtent;
  VkFormat      imageFormat;
};

class VulkanDestroyer { // DOOM!
public:
  void addImage(AllocatedImage image);
  void addBuffer(VkBuffer buffer);
  void addCommandPool(VkCommandPool pool);
  void addSemaphore(VkSemaphore semaphore);
  void addFence(VkFence fence);
  void addDescriptorPool(VkDescriptorPool pool);
  void addDescriptorSetLayout(VkDescriptorSetLayout layout);

  void flush(VkDevice device, VmaAllocator allocator);

private:
  std::vector<AllocatedImage>        images;
  std::vector<VkBuffer>              buffers;
  std::vector<VkCommandPool>         commandPools;
  std::vector<VkSemaphore>           semaphores;
  std::vector<VkFence>               fences;
  std::vector<VkDescriptorPool>      descriptorPools;
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};

struct FrameData {

  VkCommandPool   commandPool;
  VkCommandBuffer mainCommandBuffer;
  VkSemaphore     swapchainSemaphore, renderSemaphore;
  VkFence         renderFence;
};

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
  VmaAllocator               allocator;


  VulkanDestroyer vulkanDestroyer;
};

struct RenderData {

  VkQueue  graphicsQueue;
  uint32_t graphicsQueueFamily;
  // VkQueue presentQueue;

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
};
