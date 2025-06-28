#include <vulkanDestroyer.h>

void VulkanDestroyer::addImage(VkImage image, VkImageView imageView) {
  images.push_back(std::make_pair(image, imageView));
};
void VulkanDestroyer::addBuffer(VkBuffer buffer) { buffers.push_back(buffer); };
void VulkanDestroyer::addSemaphore(VkSemaphore semaphore) {
  semaphores.push_back(semaphore);
}
void VulkanDestroyer::addFence(VkFence fence) { fences.push_back(fence); }

void VulkanDestroyer::flush(VkDevice device, VmaAllocator allocator) {

  for (auto [i, iv] : images) {
    vkDestroyImageView(device, iv, nullptr);
    vmaDestroyImage(allocator, i, nullptr);
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
