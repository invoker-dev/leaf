#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <VkBootstrap.h>
#include <fmt/core.h>
#include <glm/ext/vector_float4.hpp>
#include <vector>
#include <vk_mem_alloc.h>

#include <camera.h>
#include <leafStructs.h>
#include <span>
#include <vulkan/vulkan_core.h>
#include <vulkanDestroyer.h>

constexpr bool useValidationLayers = true;

constexpr int framesInFlight = 3;

class LeafEngine {
public:
  LeafEngine();
  ~LeafEngine();

  void draw();
  void processEvent(SDL_Event& e);
  void update();

private:
  vkb::Instance              instance;
  vkb::PhysicalDevice        physicalDevice;
  vkb::Device                device;
  vkb::InstanceDispatchTable instanceDispatchTable;
  vkb::DispatchTable         dispatch;

  VkExtent2D         windowExtent;
  VkSurfaceKHR       surface;
  VkSurfaceFormatKHR surfaceFormat;
  SDL_Window*        window;

  VkQueue  graphicsQueue;
  uint32_t graphicsQueueFamily;

  vkb::Swapchain           swapchain;
  VkFormat                 swapchainImageFormat;
  std::vector<VkImage>     swapchainImages;
  std::vector<VkImageView> swapchainImageViews;
  VkExtent2D               swapchainExtent;

  VmaAllocator    allocator;
  VulkanDestroyer vulkanDestroyer;

  std::vector<VkFramebuffer> frameBuffers;
  uint32_t                   frameNumber;
  AllocatedImage             drawImage;
  VkExtent2D                 drawExtent;

  VkDescriptorPool             descriptorPool;
  VkDescriptorSetLayout        descriptorSetLayout;
  std::vector<VkDescriptorSet> descriptorSets{};

  std::vector<AllocatedBuffer> cameraBuffers{};

  imguiContext imguiContext;

  VkPipeline       pipeline;
  VkPipelineLayout pipelineLayout;

  ImmediateData immediateData;

  FrameData  frames[framesInFlight];
  FrameData& getCurrentFrame() { return frames[frameNumber % framesInFlight]; }

  glm::vec4       backgroundColor;
  glm::vec4       rectangleColor;
  GPUMeshBuffers  rectangle;
  AllocatedBuffer cameraUniformBuffer;

  Camera camera;

  void createSDLWindow();
  void initVulkan();
  void getQueues();
  void initSwapchain();
  void initCommands();
  void initSynchronization();
  void initDescriptorLayout();
  void initDescriptorPool();
  void initDescriptorSets();
  void initImGUI();
  void initPipeline();

  void drawImGUI(VkCommandBuffer cmd, VkImageView targetImage);
  void drawBackground(VkCommandBuffer cmd);
  void drawGeometry(VkCommandBuffer cmd, uint32_t imgIndex);

  AllocatedBuffer allocateBuffer(size_t allocSize, VkBufferUsageFlags usage,
                                 VmaMemoryUsage memoryUsage);
  GPUMeshBuffers  uploadMesh(std::span<uint32_t> indices,
                             std::span<Vertex>   vertices);

  void initMesh();
  void initCamera();
};
