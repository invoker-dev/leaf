#include "VkBootstrapDispatch.h"
#include <descriptorAllocator.h>
#include <fcntl.h>
#include <functional>
#include <leafInit.h>
#include <leafStructs.h>
#include <leafUtil.h>
#include <types.h>
#include <vulkan/vulkan_core.h>

DescriptorAllocator::DescriptorAllocator(vkb::DispatchTable const& dispatch) {
  this->dispatch = dispatch;
  descriptorSetLayouts.resize(FRAMES_IN_FLIGHT);
  descriptorSets.resize(FRAMES_IN_FLIGHT);
}

void DescriptorAllocator::setupLayouts() {

  std::vector<VkDescriptorSetLayoutBinding> bindings = {
      leafInit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                           VK_SHADER_STAGE_VERTEX_BIT),
  };

  VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {};
  setLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  setLayoutCreateInfo.flags        = 0;
  setLayoutCreateInfo.bindingCount = (u32)bindings.size();
  setLayoutCreateInfo.pBindings    = bindings.data();

  for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
    VkDescriptorSetLayout layout;
    vkAssert(dispatch.createDescriptorSetLayout(&setLayoutCreateInfo, nullptr,
                                                &layout));
    descriptorSetLayouts[i] = layout;
  }
}

void DescriptorAllocator::setupPools() {

  VkDescriptorPoolSize poolSizes[] = {{
      .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = FRAMES_IN_FLIGHT,
  }};

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes    = poolSizes;
  poolInfo.maxSets       = FRAMES_IN_FLIGHT;

  vkAssert(dispatch.createDescriptorPool(&poolInfo, nullptr, &descriptorPool));
}
