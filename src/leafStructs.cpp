#include <leafStructs.h>

  void VulkanDestroyer::addImage(AllocatedImage image) {
  images.push_back(image);
};
void VulkanDestroyer::addBuffer(VkBuffer buffer) { buffers.push_back(buffer); };
void VulkanDestroyer::addSemaphore(VkSemaphore semaphore) {
  semaphores.push_back(semaphore);
}
void VulkanDestroyer::addFence(VkFence fence) { fences.push_back(fence); }

void VulkanDestroyer::flush(VkDevice device, VmaAllocator allocator) {

  for (auto i : images) {
    vkDestroyImageView(device, i.imageView, nullptr);
    vmaDestroyImage(allocator, i.image, i.allocation);
  }
  images.clear();

  for (auto b : buffers) {
    vkDestroyBuffer(device, b, nullptr);
  }
  buffers.clear();

  for (auto s : semaphores) {
    vkDestroySemaphore(device, s, nullptr);
  }
  semaphores.clear();

  for (auto f : fences) {
    vkDestroyFence(device, f, nullptr);
  }
  fences.clear();
}
