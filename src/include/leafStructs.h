#pragma once

#include <VkBootstrap.h>
#include <constants.h>
#include <descriptorAllocator.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <types.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

struct AllocatedImage {
  VkImage       image;
  VkImageView   imageView;
  VmaAllocation allocation;
  VkExtent3D    extent;
  VkFormat      format;
};

struct AllocatedBuffer {
  VkBuffer          buffer;
  VmaAllocation     allocation;
  VmaAllocationInfo allocationInfo;
};

struct FrameData {
  VkCommandPool       commandPool;
  VkCommandBuffer     commandBuffer;
  VkFence             renderFence;
  VkSemaphore         imageAvailableSemaphore;
  DescriptorAllocator descriptors;
  VkDescriptorSet     descriptorSet;

  AllocatedBuffer gpuBuffer;
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

// other stuff

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

struct alignas(16) VertPushData {
  glm::mat4       model;
  glm::vec4       color;
  f32             blendFactor;
  VkDeviceAddress vertexBufferAddress;
  int             textureIndex;
};

struct alignas(16) GPUSceneData {
  glm::mat4 view;
  glm::mat4 projection;
  // glm::vec4 ambientColor;
  // glm::vec4 sunlightDirection;
  // glm::vec4 sunlightColor;
};

struct GeoSurface {
  uint32_t startIndex;
  uint32_t count;
};

struct MeshAsset {
  std::string             name;
  std::vector<GeoSurface> surfaces;
  std::vector<Vertex>     vertices;
  std::vector<u32>        indices;
  GPUMeshBuffers          meshBuffers;

  VkBuffer& getIndexBuffer() { return meshBuffers.indexBuffer.buffer; }
  VkBuffer& getVertexBuffer() { return meshBuffers.vertexBuffer.buffer; }
};
