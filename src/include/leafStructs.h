#pragma once

#include <VkBootstrap.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <types.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

constexpr bool USE_VALIDATION_LAYERS = true;

constexpr u32 DEBUG_SEVERITY = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

constexpr u32 DEBUG_TYPE =
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    // VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

constexpr u32 FRAMES_IN_FLIGHT = 3;

struct AllocatedImage {
  VkImage       image;
  VkImageView   imageView;
  VmaAllocation allocation;
  VkExtent3D    extent;
  VkFormat      format;
};

struct FrameData {
  VkCommandPool   commandPool;
  VkCommandBuffer commandBuffer;
  VkFence         renderFence;
  VkSemaphore     imageAvailableSemaphore;
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
  glm::vec4       color;
  VkDeviceAddress vertexBufferAddress;
};

struct FragPushData {
  glm::vec4 color;
};

struct CameraUBO {
  glm::mat4 view;
  glm::mat4 projection;
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
