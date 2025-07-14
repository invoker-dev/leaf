#pragma once
#include <SDL3/SDL.h>
#include <VkBootstrap.h>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

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
