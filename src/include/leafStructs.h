#pragma once

#include<VkBootstrap.h>
#include<vk_mem_alloc.h>

struct AllocatedImage {
  VkImage       image;
  VkImageView   imageView;
  VmaAllocation allocation;
  VkExtent3D    imageExtent;
  VkFormat      imageFormat;
};

struct FrameData {

  VkCommandPool   commandPool;
  VkCommandBuffer mainCommandBuffer;
  VkSemaphore     swapchainSemaphore, renderSemaphore;
  VkFence         renderFence;
};
