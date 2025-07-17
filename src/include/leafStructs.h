#pragma once

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <glm/vec4.hpp>

struct AllocatedImage {
  VkImage       image;
  VkImageView   imageView;
  VmaAllocation allocation;
  VkExtent3D    imageExtent;
  VkFormat      imageFormat;
};

struct FrameData {

  VkCommandPool   commandPool;
  VkCommandBuffer commandBuffer;
  VkSemaphore     swapchainSemaphore, renderSemaphore;
  VkFence         renderFence;
};

struct imguiContext {
  VkDescriptorPool descriptorPool;
  VkCommandPool    commandPool;
  VkCommandBuffer  commandBuffer;
};

struct ColorPushData{
  glm::vec4 color;
};
