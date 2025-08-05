#include <complex>
#include <descriptorWriter.h>
#include <fmt/base.h>
#include <fmt/format.h>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

void DescriptorWriter::writeBuffer(u32 binding, VkBuffer buffer, u32 size,
                                   u32 offset, VkDescriptorType type) {
  VkDescriptorBufferInfo& info =
      bufferInfos.emplace_back(VkDescriptorBufferInfo{
          .buffer = buffer,
          .offset = offset,
          .range  = size,
      });
  VkWriteDescriptorSet write = {};
  write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding           = binding;
  write.dstSet               = VK_NULL_HANDLE;
  write.descriptorCount      = 1;
  write.descriptorType       = type;
  write.pBufferInfo          = &info;

  writes.push_back(write);
}

void DescriptorWriter::writeImage(u32 binding, VkImageView image,
                                  VkSampler sampler, VkImageLayout layout,
                                  VkDescriptorType type) {

  VkDescriptorImageInfo& info  = imageInfos.emplace_back(VkDescriptorImageInfo{
       .sampler     = sampler,
       .imageView   = image,
       .imageLayout = layout,
  });
  VkWriteDescriptorSet   write = {};
  write.sType                  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstBinding             = binding;
  write.dstSet                 = VK_NULL_HANDLE;
  write.descriptorCount        = 1;
  write.descriptorType         = type;
  write.pImageInfo             = &info;

  writes.push_back(write);
}

void DescriptorWriter::clear() {
  imageInfos.clear();
  writes.clear();
  bufferInfos.clear();
}

void DescriptorWriter::updateSet(VkDescriptorSet set) {
  for (VkWriteDescriptorSet& write : writes) {
    write.dstSet = set;
  }

  dispatch.updateDescriptorSets((u32)writes.size(), writes.data(), 0, nullptr);
}
