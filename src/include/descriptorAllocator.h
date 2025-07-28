#include "VkBootstrapDispatch.h"
#include "leafStructs.h"
#include <VkBootstrap.h>
#include <vector>
#include <vulkan/vulkan_core.h>
class DescriptorAllocator {
public:
  DescriptorAllocator(vkb::DispatchTable const& dispatch);
  DescriptorAllocator() {}
  ~DescriptorAllocator() {}
  void setupLayouts();
  void setupPools();

  vkb::DispatchTable                 dispatch;
  VkDescriptorPool                   descriptorPool;
  std::vector<VkDescriptorSet>       descriptorSets;
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
  // std::vector<AllocatedBuffer> buffers;
};
