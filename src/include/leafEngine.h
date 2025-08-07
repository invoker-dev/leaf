#pragma once
#include "fastgltf/types.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <VkBootstrap.h>
#include <body.h>
#include <fmt/core.h>
#include <glm/ext/vector_float4.hpp>
#include <unordered_map>
#include <vector>
#include <vk_mem_alloc.h>

#include <camera.h>
#include <constants.h>
#include <descriptorAllocator.h>
#include <descriptorWriter.h>
#include <entity.h>
#include <leafStructs.h>
#include <span>
#include <vulkan/vulkan_core.h>
#include <vulkanDestroyer.h>

namespace LeafEngine {

struct VulkanCore {
  vkb::Instance              instance;
  vkb::PhysicalDevice        physicalDevice;
  vkb::Device                device;
  vkb::InstanceDispatchTable instanceDispatchTable;
  vkb::DispatchTable         dispatch;
  VmaAllocator               allocator;
  VkQueue                    graphicsQueue;
  u32                        graphicsQueueFamily;
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
  u32                      imageIndex = 0;
};

struct VulkanRenderData {

  FrameData                frames[FRAMES_IN_FLIGHT];
  u64                      frameNumber;
  AllocatedImage           drawImage;
  VkExtent2D               drawExtent;
  f32                      renderScale = 1.f;
  AllocatedImage           depthImage;
  VkPipeline               pipeline;
  VkPipelineLayout         pipelineLayout;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  glm::vec4                backgroundColor;
  glm::vec4                entityColor;

  GPUSceneData          sceneData;
  VkDescriptorSetLayout gpuSceneDataLayout;
};

struct TextureData {
  AllocatedImage                       errorImage;
  std::vector<AllocatedImage>          images;
  std::unordered_map<std::string, u32> imageMap;

  VkSampler samplerLinear;
  VkSampler samplerNearest;

  VkDescriptorSetLayout textureDescriptorLayout;
  //   VkDescriptorSet       textureSet;
  // DescriptorAllocator descriptors;
};

struct SimulationData {

  std::vector<Body> bodies;
  f32               globalScale   = 1;
  f32               planetScale   = 1;
  f32               distanceScale = 1e-8;
  f32               timeScale     = 1;
  Light             sunlight;
};

class Engine {
public:
  Engine();
  ~Engine();

  void draw();
  void update(f64 dt);
  void processEvent(SDL_Event& event);

  VulkanCore       core;
  VulkanSurface    surface;
  VulkanSwapchain  swapchain;
  VulkanRenderData renderData;
  TextureData      textureData;

  VulkanDestroyer vulkanDestroyer;

  ImmediateData immediateData;
  ImguiContext  imguiContext;

  Camera camera;
  u32    targetIndex;

  SimulationData simulationData;

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void initSwapchain();
  void initCommands();
  void initSynchronization();
  void initDescriptors();
  void initImGUI();
  void initPipeline();
  void initSolarSystem();
  void initSceneData();

  void prepareFrame();
  void submitFrame();
  void drawImGUI(VkCommandBuffer cmd, VkImageView targetImage);
  void drawBackground(VkCommandBuffer cmd);
  void drawGeometry(VkCommandBuffer cmd);

  void            createSwapchain(u32 width, u32 height);
  void            resizeSwapchain(u32 width, u32 height);
  AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage,
                               VmaMemoryUsage memoryUsage);

  AllocatedImage createImage(VkExtent3D extent, VkFormat format,
                             VkImageUsageFlags usage, bool mipmapped = false);

  AllocatedImage createImage(void* data, VkExtent3D extent, VkFormat format,
                             VkImageUsageFlags usage, bool mipmapped = false);

  GPUMeshBuffers uploadMesh(std::span<u32> indices, std::span<Vertex> vertices);

  FrameData& getCurrentFrame() {
    return renderData.frames[renderData.frameNumber % FRAMES_IN_FLIGHT];
  }
};

} // namespace LeafEngine
