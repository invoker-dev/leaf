#include "leafEngine.h"
#include <fmt/base.h>
#include <leafStructs.h>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>
// VulkanDestroyer
void VulkanDestroyer::addImage(AllocatedImage image) {
  images.push_back(image);
};

void VulkanDestroyer::addBuffer(VkBuffer buffer) { buffers.push_back(buffer); };
void VulkanDestroyer::addSemaphore(VkSemaphore semaphore) {
  semaphores.push_back(semaphore);
}
void VulkanDestroyer::addFence(VkFence fence) { fences.push_back(fence); }

void VulkanDestroyer::addCommandPool(VkCommandPool pool) {
  commandPools.push_back(pool);
}

void VulkanDestroyer::addDescriptorPool(VkDescriptorPool pool) {
  descriptorPools.push_back(pool);
}
void VulkanDestroyer::addDescriptorSetLayout(VkDescriptorSetLayout layout) {
  descriptorSetLayouts.push_back(layout);
}
void VulkanDestroyer::flush(VkDevice device, VmaAllocator allocator) {

  for (auto p : commandPools) {
    vkDestroyCommandPool(device, p, nullptr);
  }

  for (auto i : images) {
    vkDestroyImageView(device, i.imageView, nullptr);
    vmaDestroyImage(allocator, i.image, i.allocation);
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

  for (auto p : descriptorPools) {
    vkDestroyDescriptorPool(device, p, nullptr);
  }

  for (auto l : descriptorSetLayouts) {
    vkDestroyDescriptorSetLayout(device, l, nullptr);
  }
}

// DescriptorLayoutBuilder
void DescriptorLayoutBuilder::addBinding(uint32_t         binding,
                                         VkDescriptorType type) {

  VkDescriptorSetLayoutBinding layoutBinding = {};

  layoutBinding.binding         = binding;
  layoutBinding.descriptorCount = 1;
  layoutBinding.descriptorType  = type;
  layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  bindings.push_back(layoutBinding);
}

void DescriptorLayoutBuilder::clear() { bindings.clear(); }

VkDescriptorSetLayout
DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages,
                               void*                            pNext,
                               VkDescriptorSetLayoutCreateFlags flags) {

  for (auto& b : bindings) {
    b.stageFlags |= shaderStages;
  }

  VkDescriptorSetLayoutCreateInfo info = {};
  info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.pNext        = pNext;
  info.pBindings    = bindings.data();
  info.bindingCount = (uint32_t)bindings.size();
  // info.bindingCount = 1;
  info.flags        = flags;

  VkDescriptorSetLayout set;
  vkAssert(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

  return set;
}
// descriptor allocator

DescriptorAllocator::DescriptorAllocator(VkDevice device, uint32_t maxSets,
                                         std::span<PoolSizeRatio> poolRatios) {
  this->device = device;

  std::vector<VkDescriptorPoolSize> poolSizes;
  for (PoolSizeRatio ratio : poolRatios) {
    VkDescriptorPoolSize dps = {};
    dps.type                 = ratio.type;
    dps.descriptorCount      = static_cast<uint32_t>(ratio.ratio * maxSets);

    poolSizes.push_back(dps);
  }

  VkDescriptorPoolCreateInfo info = {};

  info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.flags         = 0;
  info.maxSets       = maxSets;
  info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  info.pPoolSizes    = poolSizes.data();

  vkAssert(vkCreateDescriptorPool(device, &info, nullptr, &pool));
}

DescriptorAllocator::~DescriptorAllocator() {
  vkDestroyDescriptorPool(device, pool, nullptr);
}
void DescriptorAllocator::clear() { vkResetDescriptorPool(device, pool, 0); }

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout) {

  VkDescriptorSetAllocateInfo info = {};
  info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.descriptorPool     = pool;
  info.descriptorSetCount = 1;
  info.pSetLayouts        = &layout;

  VkDescriptorSet set;

  vkAssert(vkAllocateDescriptorSets(device, &info, &set));

  return set;
}
