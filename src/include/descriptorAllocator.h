#pragma once
#include "VkBootstrapDispatch.h"
#include <VkBootstrap.h>
#include <span>
#include <types.h>
#include <vector>
#include <vulkan/vulkan_core.h>

class DescriptorAllocator {
public:

  DescriptorAllocator(){};
  struct PoolSizeRatio {
    VkDescriptorType type;
    float            ratio;
  };

  void            init(vkb::DispatchTable const& dispatch, u32 initialSets,
                       std::span<PoolSizeRatio> poolRatios);
  void            clearPools();
  void            destroyPools();
  VkDescriptorSet allocate(VkDescriptorSetLayout layout, void* pNext);

  VkDescriptorPool getPool();
  VkDescriptorPool createPool(u32                      setCount,
                              std::span<PoolSizeRatio> poolRatios);

  vkb::DispatchTable dispatch;

  std::vector<PoolSizeRatio>    ratios;
  std::vector<VkDescriptorPool> fullPools;
  std::vector<VkDescriptorPool> readyPools;
  u32                           setsPerPool;
  u32                           maxSetCount = 4096;
};
