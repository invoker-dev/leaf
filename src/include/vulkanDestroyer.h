#pragma once
#include "VkBootstrapDispatch.h"
#include <VkBootstrap.h>
#include <leafStructs.h>
#include <vector>

class VulkanDestroyer { // DOOM!
public:
  void addImage(AllocatedImage const& image);
  void addAllocatedBuffer(AllocatedBuffer const& buffer);
  void addCommandPool(VkCommandPool const& pool);
  void addSemaphore(VkSemaphore const& semaphore);
  void addFence(VkFence const& fence);
  void addDescriptorPool(VkDescriptorPool const& pool);
  void addDescriptorSetLayout(VkDescriptorSetLayout const& layout);
  void addPipeline(VkPipeline const& pipeline);
  void addPipelineLayout(VkPipelineLayout const& pipelineLayout);

  void flush(vkb::DispatchTable const& dispatch, VmaAllocator allocator);

private:
  std::vector<AllocatedImage>        images;
  std::vector<AllocatedBuffer>       buffers;
  std::vector<VkCommandPool>         commandPools;
  std::vector<VkSemaphore>           semaphores;
  std::vector<VkFence>               fences;
  std::vector<VkDescriptorPool>      descriptorPools;
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
  std::vector<VkPipeline>            pipelines;
  std::vector<VkPipelineLayout>      pipelineLayouts;
};
