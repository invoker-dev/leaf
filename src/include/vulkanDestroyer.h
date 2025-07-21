#pragma once
#include "VkBootstrapDispatch.h"
#include <VkBootstrap.h>
#include <leafStructs.h>

class VulkanDestroyer { // DOOM!
public:
  void addImage(AllocatedImage image);
  void addAllocatedBuffer(AllocatedBuffer buffer);
  void addCommandPool(VkCommandPool pool);
  void addSemaphore(VkSemaphore semaphore);
  void addFence(VkFence fence);
  void addDescriptorPool(VkDescriptorPool pool);
  void addDescriptorSetLayout(VkDescriptorSetLayout layout);
  void addPipeline(VkPipeline pipeline);
  void addPipelineLayout(VkPipelineLayout pipelineLayout);

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
