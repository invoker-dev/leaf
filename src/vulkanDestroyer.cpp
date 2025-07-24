#include "VkBootstrapDispatch.h"
#include <vulkanDestroyer.h>

void VulkanDestroyer::addImage(AllocatedImage const& image) {
  images.push_back(image);
};

void VulkanDestroyer::addAllocatedBuffer(AllocatedBuffer const& buffer) {
  buffers.push_back(buffer);
};
void VulkanDestroyer::addSemaphore(VkSemaphore const& semaphore) {
  semaphores.push_back(semaphore);
}
void VulkanDestroyer::addFence(VkFence const& fence) {
  fences.push_back(fence);
}

void VulkanDestroyer::addCommandPool(VkCommandPool const& pool) {
  commandPools.push_back(pool);
}

void VulkanDestroyer::addDescriptorPool(VkDescriptorPool const& pool) {
  descriptorPools.push_back(pool);
}
void VulkanDestroyer::addDescriptorSetLayout(
    VkDescriptorSetLayout const& layout) {
  descriptorSetLayouts.push_back(layout);
}

void VulkanDestroyer::addPipeline(VkPipeline const& pipeline) {
  pipelines.push_back(pipeline);
}

void VulkanDestroyer::addPipelineLayout(
    VkPipelineLayout const& pipelineLayout) {
  pipelineLayouts.push_back(pipelineLayout);
}
void VulkanDestroyer::flush(vkb::DispatchTable const& dispatch,
                            VmaAllocator              allocator) {

  for (auto p : commandPools) {
    dispatch.destroyCommandPool(p, nullptr);
  }

  for (auto i : images) {
    dispatch.destroyImageView(i.imageView, nullptr);
    vmaDestroyImage(allocator, i.image, i.allocation);
  }
  images.clear();

  for (auto b : buffers) {
    vmaDestroyBuffer(allocator, b.buffer, b.allocation);
  }
  buffers.clear();

  for (auto s : semaphores) {
    dispatch.destroySemaphore(s, nullptr);
  }
  semaphores.clear();

  for (auto f : fences) {
    dispatch.destroyFence(f, nullptr);
  }
  fences.clear();

  for (auto p : descriptorPools) {
    dispatch.destroyDescriptorPool(p, nullptr);
  }

  for (auto l : descriptorSetLayouts) {
    dispatch.destroyDescriptorSetLayout(l, nullptr);
  }

  for (auto p : pipelines) {
    dispatch.destroyPipeline(p, nullptr);
  }
  for (auto l : pipelineLayouts) {
    dispatch.destroyPipelineLayout(l, nullptr);
  }
}
