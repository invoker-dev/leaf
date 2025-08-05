#include "VkBootstrapDispatch.h"
#include "leafStructs.h"
#include <VkBootstrap.h>
#include <deque>
#include <span>
#include <types.h>
#include <vector>
#include <vulkan/vulkan_core.h>
class DescriptorWriter {
public:
  void init(vkb::DispatchTable dispatch) { this->dispatch = dispatch; }
  void writeImage(u32 binding, VkImageView image, VkSampler sampler,
                  VkImageLayout layout, VkDescriptorType type);

  void writeImages(u32 binding, std::span<AllocatedImage> image,
                   VkSampler sampler, VkImageLayout layout,
                   VkDescriptorType type);

  void writeBuffer(u32 binding, VkBuffer buffer, u32 size, u32 offset,
                   VkDescriptorType type);
  void clear();
  void updateSet(VkDescriptorSet set);

  vkb::DispatchTable dispatch;

  std::deque<VkDescriptorImageInfo>  imageInfos;
  std::vector<VkDescriptorImageInfo> arrayImageInfos;
  std::deque<VkDescriptorBufferInfo> bufferInfos;
  std::vector<VkWriteDescriptorSet>  writes;
};
