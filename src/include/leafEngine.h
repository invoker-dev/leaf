#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <VkBootstrap.h>
#include <fmt/core.h>
#include <glm/ext/vector_float4.hpp>
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

  VulkanDestroyer vulkanDestroyer;

  ImmediateData immediateData;
  ImguiContext  imguiContext;

  Camera camera;

  std::vector<Entity> entities;

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void initSwapchain();
  void initCommands();
  void initSynchronization();
  void initDescriptors();
  void initImGUI();
  void initPipeline();
  void initEntities();
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

  AllocatedImage  createImage(VkExtent3D size, VkFormat format,
                              VkImageUsageFlags usage, bool mipmapped = false);

  AllocatedImage  createImage(void* data, VkExtent3D size, VkFormat format,
                              VkImageUsageFlags usage, bool mipmapped = false);

  GPUMeshBuffers uploadMesh(std::span<u32> indices, std::span<Vertex> vertices);

  FrameData& getCurrentFrame() {
    return renderData.frames[renderData.frameNumber % FRAMES_IN_FLIGHT];
  }
};

} // namespace LeafEngine
