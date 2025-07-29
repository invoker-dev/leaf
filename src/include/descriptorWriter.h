#include "VkBootstrapDispatch.h"
#include <VkBootstrap.h>
#include <deque>
#include <types.h>
#include <vector>
#include <vulkan/vulkan_core.h>
class DescriptorWriter {
public:

  DescriptorWriter(){};
  DescriptorWriter(vkb::DispatchTable dispatch) { this->dispatch = dispatch; }
  void writeImage(u32 binding, VkImageView image, VkSampler sampler,
                  VkImageLayout layout, VkDescriptorType type);
  void writeBuffer(u32 binding, VkBuffer buffer, u32 size, u32 offset,
                   VkDescriptorType type);
  void clear();
  void updateSet(VkDescriptorSet set);

  vkb::DispatchTable dispatch;

  std::deque<VkDescriptorImageInfo>  imageInfos;
  std::deque<VkDescriptorBufferInfo> bufferInfos;
  std::vector<VkWriteDescriptorSet>  writes;
};
