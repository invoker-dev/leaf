#pragma once

#include <VkBootstrap.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

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

struct ImmediateData {
  VkCommandPool   commandPool;
  VkCommandBuffer commandBuffer;
  VkFence         fence;
};

struct ImguiContext {
  VkDescriptorPool descriptorPool;
  VkCommandPool    commandPool;
  VkCommandBuffer  commandBuffer;
};

struct AllocatedBuffer {
  VkBuffer          buffer;
  VmaAllocation     allocation;
  VmaAllocationInfo allocationInfo;
};

// types

struct Vertex {
  glm::vec3 position;
  float     uv_x;
  glm::vec3 normal;
  float     uv_y;
  glm::vec4 color;
};

struct GPUMeshBuffers {
  AllocatedBuffer indexBuffer;
  AllocatedBuffer vertexBuffer;
  VkDeviceAddress vertexBufferAddress;
};

struct VertPushData {
  glm::mat4       model;
  VkDeviceAddress vertexBufferAddress;
};

struct FragPushData {
  glm::vec4 color;
};

struct CameraUBO {
  glm::mat4 view;
  glm::mat4 projection;
};
