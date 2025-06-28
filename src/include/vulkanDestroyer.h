#pragma once
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

class VulkanDestroyer { // DOOM!
public:
  void addImage(VkImage image, VkImageView imageView);
  void addBuffer(VkBuffer buffer);
  void addSemaphore(VkSemaphore semaphore);
  void addFence(VkFence fence);

  void flush(VkDevice device, VmaAllocator allocator);
private:
  std::vector<std::pair<VkImage, VkImageView>> images;
  std::vector<VkBuffer>                        buffers;
  std::vector<VkSemaphore>                     semaphores;
  std::vector<VkFence>                         fences;
};
