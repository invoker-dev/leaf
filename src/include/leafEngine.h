#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <VkBootstrap.h>
#include <cstdint>
#include <fmt/core.h>
#include <glm/ext/vector_float4.hpp>
#include <vector>
#include <vk_mem_alloc.h>

#include <camera.h>
#include <cube.h>
#include <leafStructs.h>
#include <span>
#include <vulkan/vulkan_core.h>
#include <vulkanDestroyer.h>

constexpr bool     useValidationLayers = true;
constexpr uint32_t framesInFlight      = 3;

namespace LeafEngine {

struct VulkanCore {
  vkb::Instance              instance;
  vkb::PhysicalDevice        physicalDevice;
  vkb::Device                device;
  vkb::InstanceDispatchTable instanceDispatchTable;
  vkb::DispatchTable         dispatch;
  VmaAllocator               allocator;
  VkQueue                    graphicsQueue;
  uint32_t                   graphicsQueueFamily;
};

struct VulkanSurface {
  VkExtent2D         extent;
  VkSurfaceKHR       surface;
  VkSurfaceFormatKHR surfaceFormat;
  SDL_Window*        window;
  bool               captureMouse;
};

struct VulkanSwapchain {

  vkb::Swapchain           vkbSwapchain;
  VkFormat                 imageFormat;
  std::vector<VkImage>     images;
  std::vector<VkImageView> imageViews;
  VkExtent2D               extent;
  bool                     resize;
};

struct VulkanRenderData {

  // std::vector<VkFramebuffer>   frameBuffers;
  FrameData                    frames[framesInFlight];
  uint64_t                     frameNumber;
  AllocatedImage               drawImage;
  VkExtent2D                   drawExtent;
  float                        renderScale = 1.f;
  AllocatedImage               depthImage;
  VkDescriptorPool             descriptorPool;
  VkDescriptorSetLayout        descriptorSetLayout;
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<AllocatedBuffer> cameraBuffers;
  VkPipeline                   pipeline;
  VkPipelineLayout             pipelineLayout;

  glm::vec4 backgroundColor;
};

class Engine {
public:
  Engine();
  ~Engine();

  void draw();
  void update();
  void processEvent(SDL_Event& e);

  VulkanCore       core;
  VulkanSurface    surface;
  VulkanSwapchain  swapchain;
  VulkanRenderData renderData;

  VulkanDestroyer vulkanDestroyer;

  ImmediateData immediateData;
  ImguiContext  imguiContext;

  Camera camera;

  CubeSystem cubeSystem;

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void initSwapchain();
  void initCommands();
  void initSynchronization();
  void initDepthTest();
  void initDescriptorLayout();
  void initDescriptorPool();
  void initDescriptorSets();
  void initImGUI();
  void initPipeline();
  void initMesh();
  void initCamera();
  void initCubes();

  void drawImGUI(VkCommandBuffer cmd, VkImageView targetImage);
  void drawBackground(VkCommandBuffer cmd);
  void drawGeometry(VkCommandBuffer cmd, uint32_t imgIndex);

  void            createSwapchain(uint32_t width, uint32_t height);
  void            resizeSwapchain(uint32_t width, uint32_t height);
  AllocatedBuffer allocateBuffer(size_t allocSize, VkBufferUsageFlags usage,
                                 VmaMemoryUsage memoryUsage);
  GPUMeshBuffers  uploadMesh(std::span<uint32_t> indices,
                             std::span<Vertex>   vertices);

  FrameData& getCurrentFrame() {
    return renderData.frames[renderData.frameNumber % framesInFlight];
  }
};

} // namespace LeafEngine
