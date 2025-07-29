#include "VkBootstrapDispatch.h"
#include <descriptorAllocator.h>
#include <fcntl.h>
#include <leafInit.h>
#include <leafStructs.h>
#include <leafUtil.h>
#include <types.h>
#include <vulkan/vulkan_core.h>

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout,
                                              void*                 pNext) {

  VkDescriptorPool poolToUse = getPool();

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.pNext              = pNext;
  allocInfo.descriptorPool     = poolToUse;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = &layout;

  VkDescriptorSet ds;
  VkResult        result = dispatch.allocateDescriptorSets(&allocInfo, &ds);

  if (result == VK_ERROR_OUT_OF_POOL_MEMORY ||
      result == VK_ERROR_FRAGMENTED_POOL) {
    fullPools.push_back(poolToUse);
    poolToUse                = getPool();
    allocInfo.descriptorPool = poolToUse;

    VK_ASSERT(dispatch.allocateDescriptorSets(&allocInfo, &ds));
  }

  readyPools.push_back(poolToUse);
  return ds;
}

void DescriptorAllocator::init(vkb::DispatchTable const& dispatch,
                               u32                       initialSets,
                               std::span<PoolSizeRatio>  poolRatios) {

  this->dispatch = dispatch;
  ratios.clear();
  for (auto r : poolRatios) {
    ratios.push_back(r);
  }
  VkDescriptorPool newPool = createPool(initialSets, poolRatios);
  setsPerPool              = initialSets *= 1.5;

  readyPools.push_back(newPool);
}
void DescriptorAllocator::clearPools() {
  for (auto p : readyPools) {
    dispatch.resetDescriptorPool(p, 0);
  }
  for (auto p : fullPools) {
    dispatch.resetDescriptorPool(p, 0);
    readyPools.push_back(p);
  }
  fullPools.clear();
}
void DescriptorAllocator::destroyPools() {

  for (auto p : readyPools) {
    dispatch.destroyDescriptorPool(p, nullptr);
  }
  readyPools.clear();
  for (auto p : fullPools) {
    dispatch.destroyDescriptorPool(p, nullptr);
  }
  fullPools.clear();
}

VkDescriptorPool DescriptorAllocator::getPool() {
  VkDescriptorPool newPool;
  if (readyPools.size() != 0) {
    newPool = readyPools.back();
    readyPools.pop_back();
  } else {
    newPool     = createPool(setsPerPool, ratios);
    setsPerPool *= 1.5;
    if (setsPerPool > (maxSetCount)) {
      setsPerPool = maxSetCount;
    }
  }
  return newPool;
}

VkDescriptorPool
DescriptorAllocator::createPool(u32                      setCount,
                                std::span<PoolSizeRatio> poolRatios) {

  std::vector<VkDescriptorPoolSize> poolSizes;
  for (PoolSizeRatio ratio : poolRatios) {
    poolSizes.push_back(VkDescriptorPoolSize{
        .type            = ratio.type,
        .descriptorCount = (u32)(ratio.ratio * setCount),
    });
  }

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags         = 0;
  poolInfo.maxSets       = setCount;
  poolInfo.poolSizeCount = (u32)poolSizes.size();
  poolInfo.pPoolSizes    = poolSizes.data();

  VkDescriptorPool newPool;
  dispatch.createDescriptorPool(&poolInfo, nullptr, &newPool);
  return newPool;
}
