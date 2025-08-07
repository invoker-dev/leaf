// Actual todo
// TODO :: glTF loading fix
// TODO :: fix the pipelines for multiple shaders
// TODO: Stencil Testing
// TODO: mipmapping, texturesampling
// TODO: Lighting
// TODO: Normal maps
// TODO: Various Post processing
// CONTINOUS TODO: Real Architecture
#include "entity.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <body.h>
#include <cmath>
#include <constants.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <descriptorAllocator.h>
#include <filesystem>
#include <fmt/base.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/packing.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <iterator>
#include <leafEngine.h>
#include <leafGltf.h>
#include <leafInit.h>
#include <leafStructs.h>
#include <leafUtil.h>
#include <pipelineBuilder.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_truetype.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace LeafEngine {

Engine::Engine() {

  core       = {};
  swapchain  = {};
  surface    = {};
  renderData = {};

  createSDLWindow();
  initVulkan();
  getQueues();
  initSwapchain();
  initCommands();
  initSynchronization();

  // TODO: This order is not fine
  initSceneData();
  initDescriptors();
  initImGUI();
  initPipeline();
  initSolarSystem();
};
Engine::~Engine() {

  VK_ASSERT(core.dispatch.deviceWaitIdle());

  // destroys primitives (images, buffers, fences etc)

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  vulkanDestroyer.flush(core.dispatch, core.allocator);
  vmaDestroyAllocator(core.allocator);
  vkb::destroy_swapchain(swapchain.vkbSwapchain);
  vkb::destroy_device(core.device);
  vkb::destroy_surface(core.instance, surface.surface);
  vkb::destroy_instance(core.instance);
  SDL_DestroyWindow(surface.window);
  SDL_Quit();
}

void Engine::createSDLWindow() {

  SDL_WindowFlags sdlFlags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  surface.window = SDL_CreateWindow("LeafEngine", 1920, 1080, sdlFlags);
  if (!surface.window) {
    fmt::print("failed to init SDL window: {}\n", SDL_GetError());
    std::exit(-1);
  }

  if (!SDL_ShowWindow(surface.window)) {
    fmt::print("failed to show SDL window: {}\n", SDL_GetError());
    std::exit(-1);
  }

  surface.captureMouse = true;
  SDL_SetWindowRelativeMouseMode(surface.window, surface.captureMouse);
}

void Engine::initVulkan() {

  u32 count = 0;
  SDL_Vulkan_GetInstanceExtensions(&count);
  const char* const*       arr = SDL_Vulkan_GetInstanceExtensions(&count);
  std::vector<const char*> sdlExtensions(arr, arr + count);

  vkb::InstanceBuilder builder;

  fmt::println("extensions");
  for (auto ext : sdlExtensions) {
    fmt::println("{}", ext);
  }

  builder.set_app_name("leaf")
      .require_api_version(1, 3, 0)
      .use_default_debug_messenger()
      .request_validation_layers(USE_VALIDATION_LAYERS)
      .set_debug_messenger_severity(DEBUG_SEVERITY)
      .set_debug_messenger_type(DEBUG_TYPE)
      .enable_extensions(sdlExtensions);

  auto returnedInstance = builder.build();

  if (!returnedInstance) {
    fmt::println("Failed to build vkbInstance: {}",
                 returnedInstance.error().message());
    std::exit(-1);
  }

  core.instance = returnedInstance.value();

  core.instanceDispatchTable = core.instance.make_table();
  bool result = SDL_Vulkan_CreateSurface(surface.window, core.instance.instance,
                                         nullptr, &surface.surface);
  if (!result) {
    fmt::println("failed to create SDL surface: {}", SDL_GetError());
  }

  VkPhysicalDeviceVulkan13Features features13{};
  features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features13.dynamicRendering = true; // allows us to skip render pass
  features13.synchronization2 = true;

  VkPhysicalDeviceVulkan12Features features12{};
  features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing  = true;

  vkb::PhysicalDeviceSelector selector{core.instance};
  auto returnedPhysicalDevice = selector.set_minimum_version(1, 3)
                                    .set_required_features_13(features13)
                                    .set_required_features_12(features12)
                                    .set_surface(surface.surface)
                                    .select();

  if (!returnedPhysicalDevice) {
    fmt::println("Failed to find physical device: {}",
                 returnedPhysicalDevice.error().message());
    std::exit(-1);
  }

  core.physicalDevice = returnedPhysicalDevice.value();

  vkb::DeviceBuilder deviceBuilder{core.physicalDevice};
  auto               returnedDevice = deviceBuilder.build();

  if (!returnedDevice) {
    fmt::println("Failed to build device: {}",
                 returnedDevice.error().message());
    std::exit(-1);
  }

  core.device   = returnedDevice.value();
  core.dispatch = core.device.make_table();

  fmt::println("PD {}", (void*)core.physicalDevice.physical_device);

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice         = core.physicalDevice;
  allocatorInfo.device                 = core.device.device;
  allocatorInfo.instance               = core.instance.instance;
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  fmt::println("PD {}", (void*)core.physicalDevice.physical_device);
  fmt::println("PD {}", (void*)core.physicalDevice);
  vmaCreateAllocator(&allocatorInfo, &core.allocator);
}

void Engine::getQueues() {
  auto gq = core.device.get_queue(vkb::QueueType::graphics);
  if (!gq.has_value()) {
    fmt::println("failed to get graphics queue: {}", gq.error().message());
    std::exit(-1);
  }
  core.graphicsQueue = gq.value();
  core.graphicsQueueFamily =
      core.device.get_queue_index(vkb::QueueType::graphics).value();
}

void Engine::createSwapchain(u32 width, u32 height) {

  fmt::println("Window size:   [{} {}]", width, height);

  vkb::SwapchainBuilder swapchainBuilder{core.physicalDevice, core.device,
                                         surface.surface};

  swapchain.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;

  auto returnedSwapchain =
      swapchainBuilder.set_desired_format(surface.surfaceFormat)
          .set_desired_format(VkSurfaceFormatKHR{
              .format     = swapchain.imageFormat,
              .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
          .set_desired_min_image_count(FRAMES_IN_FLIGHT)
          .set_desired_extent(width, height)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build();

  if (!returnedSwapchain) {
    fmt::println("failed to build swapchain: {}",
                 returnedSwapchain.error().message());
    std::exit(-1);
  }

  swapchain.vkbSwapchain = returnedSwapchain.value();
  swapchain.imageViews   = swapchain.vkbSwapchain.get_image_views().value();
  swapchain.images       = swapchain.vkbSwapchain.get_images().value();
  swapchain.extent       = swapchain.vkbSwapchain.extent;

  initSynchronization();
}
void Engine::initSwapchain() {

  int width, height;
  SDL_GetWindowSizeInPixels(surface.window, &width, &height);
  surface.extent = {static_cast<u32>(width), static_cast<u32>(height)};

  createSwapchain(width, height);

  VkExtent3D drawImageExtent = {};
  drawImageExtent.width      = surface.extent.width;
  drawImageExtent.height     = surface.extent.height;
  drawImageExtent.depth      = 1;

  renderData.drawImage        = {};
  renderData.drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
  renderData.drawImage.extent = drawImageExtent;

  VkImageUsageFlags drawImageUsages = {};
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo renderImageCreateInfo = leafInit::imageCreateInfo(
      renderData.drawImage.format, drawImageUsages, drawImageExtent);

  VmaAllocationCreateInfo renderImageAllocationInfo = {};
  renderImageAllocationInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
  renderImageAllocationInfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vmaCreateImage(core.allocator, &renderImageCreateInfo,
                 &renderImageAllocationInfo, &renderData.drawImage.image,
                 &renderData.drawImage.allocation, nullptr);

  VkImageViewCreateInfo renderImageViewCreateInfo =
      leafInit::imageViewCreateInfo(renderData.drawImage.format,
                                    renderData.drawImage.image,
                                    VK_IMAGE_ASPECT_COLOR_BIT);

  VK_ASSERT(core.dispatch.createImageView(&renderImageViewCreateInfo, nullptr,
                                          &renderData.drawImage.imageView));

  renderData.depthImage        = {};
  renderData.depthImage.format = VK_FORMAT_D32_SFLOAT;        // TODO subject to
                                                              // change
  renderData.depthImage.extent = renderData.drawImage.extent; // ?maybe

  VkImageUsageFlags depthImageUsages = {};
  depthImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  depthImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  depthImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VkImageCreateInfo depthImageInfo =
      leafInit::imageCreateInfo(renderData.depthImage.format, depthImageUsages,
                                renderData.drawImage.extent);

  VmaAllocationCreateInfo depthAllocInfo = {};
  depthAllocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
  depthAllocInfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vmaCreateImage(core.allocator, &depthImageInfo, &depthAllocInfo,
                 &renderData.depthImage.image,
                 &renderData.depthImage.allocation, nullptr);

  VkImageViewCreateInfo depthImageViewCreateInfo =
      leafInit::imageViewCreateInfo(renderData.depthImage.format,
                                    renderData.depthImage.image,
                                    VK_IMAGE_ASPECT_DEPTH_BIT);

  core.dispatch.createImageView(&depthImageViewCreateInfo, nullptr,
                                &renderData.depthImage.imageView);
}

void Engine::initCommands() {

  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cmdPoolInfo.queueFamilyIndex = core.graphicsQueueFamily;

  for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {

    VK_ASSERT(core.dispatch.createCommandPool(
        &cmdPoolInfo, nullptr, &renderData.frames[i].commandPool));

    vulkanDestroyer.addCommandPool(renderData.frames[i].commandPool);

    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = renderData.frames[i].commandPool;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_ASSERT(core.dispatch.allocateCommandBuffers(
        &cmdAllocInfo, &renderData.frames[i].commandBuffer));
  }
  // immediate
  VK_ASSERT(core.dispatch.createCommandPool(&cmdPoolInfo, nullptr,
                                            &immediateData.commandPool));

  VkCommandBufferAllocateInfo immCmdAllocInfo = {};
  immCmdAllocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  immCmdAllocInfo.commandPool = immediateData.commandPool;
  immCmdAllocInfo.commandBufferCount = 1;
  immCmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  VK_ASSERT(core.dispatch.allocateCommandBuffers(&immCmdAllocInfo,
                                                 &immediateData.commandBuffer));
}

void Engine::initSynchronization() {

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start as signaled

  VkSemaphoreCreateInfo semaphoreInfo = {};

  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreInfo.flags = 0;

  u32 imageCount = swapchain.vkbSwapchain.image_count;
  renderData.renderFinishedSemaphores.resize(imageCount);

  for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
    VK_ASSERT(core.dispatch.createFence(&fenceInfo, nullptr,
                                        &renderData.frames[i].renderFence));
    vulkanDestroyer.addFence(renderData.frames[i].renderFence);

    VK_ASSERT(core.dispatch.createSemaphore(
        &semaphoreInfo, nullptr,
        &renderData.frames[i].imageAvailableSemaphore));

    vulkanDestroyer.addSemaphore(renderData.frames[i].imageAvailableSemaphore);
  }

  for (size_t i = 0; i < imageCount; i++) {

    VK_ASSERT(core.dispatch.createSemaphore(
        &semaphoreInfo, nullptr, &renderData.renderFinishedSemaphores[i]));

    vulkanDestroyer.addSemaphore(renderData.renderFinishedSemaphores[i]);
  }

  VK_ASSERT(
      core.dispatch.createFence(&fenceInfo, nullptr, &immediateData.fence));
  vulkanDestroyer.addFence(immediateData.fence);
}

void Engine::initImGUI() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(1.5);
  style.FontScaleDpi = 1.5;

  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets       = 1000;
  poolInfo.poolSizeCount = (u32)std::size(pool_sizes);
  poolInfo.pPoolSizes    = pool_sizes;

  VK_ASSERT(core.dispatch.createDescriptorPool(&poolInfo, nullptr,
                                               &imguiContext.descriptorPool));

  VkPipelineCreateInfoKHR pipelineInfo = {};

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance                    = core.instance;
  initInfo.PhysicalDevice              = core.physicalDevice;
  initInfo.Device                      = core.device;
  initInfo.QueueFamily                 = core.graphicsQueueFamily;
  initInfo.Queue                       = core.graphicsQueue;
  initInfo.PipelineCache               = VK_NULL_HANDLE;
  initInfo.DescriptorPool              = imguiContext.descriptorPool;
  initInfo.Subpass                     = 0;
  initInfo.MinImageCount               = FRAMES_IN_FLIGHT;
  initInfo.ImageCount                  = FRAMES_IN_FLIGHT;
  initInfo.UseDynamicRendering         = true;
  initInfo.PipelineRenderingCreateInfo = {};
  initInfo.PipelineRenderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats =
      &swapchain.imageFormat;
  initInfo.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
  initInfo.Allocator       = nullptr;
  initInfo.CheckVkResultFn = VK_IMGUI_ASSERT;

  if (!ImGui_ImplSDL3_InitForVulkan(surface.window)) {
    fmt::println("ImGui_ImplSDL3_InitForVulkan failed");
  }
  if (!ImGui_ImplVulkan_Init(&initInfo)) {
    fmt::println("failed to init IMGUI");
  }

  vulkanDestroyer.addDescriptorPool(imguiContext.descriptorPool);
}

void Engine::initPipeline() {

  VkShaderModule vertexShader =
      leafUtil::loadShaderModule("triangleMesh.vert", core.dispatch);
  VkShaderModule fragmentShader =
      leafUtil::loadShaderModule("triangle.frag", core.dispatch);

  VkPushConstantRange pushConstantRanges[1];

  pushConstantRanges[0].offset     = 0;
  pushConstantRanges[0].size       = sizeof(VertPushData);
  pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayout layouts[] = {renderData.gpuSceneDataLayout};
  // // align by 16 bytes
  // constexpr size_t fragOffset      = (sizeof(VertPushData) + 15) & ~15;
  // pushConstantRanges[1].offset     = fragOffset;
  // pushConstantRanges[1].size       = sizeof(FragPushData);
  // pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.flags = 0;
  pipelineLayoutInfo.setLayoutCount         = 1;
  pipelineLayoutInfo.pSetLayouts            = layouts;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges    = pushConstantRanges;

  VK_ASSERT(core.dispatch.createPipelineLayout(&pipelineLayoutInfo, nullptr,
                                               &renderData.pipelineLayout));

  PipelineBuilder pipelineBuilder =
      PipelineBuilder(core.dispatch)
          .setLayout(renderData.pipelineLayout)
          .setShaders(vertexShader, fragmentShader)
          .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
          .setPolygonMode(VK_POLYGON_MODE_FILL)
          // .setPolygonMode(VK_POLYGON_MODE_LINE) // wireframe
          .setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
          .disableMultiSampling()
          .enableBlendingAlphaBlend()
          // .enableBlendingAdditive()
          .setDepthTest(true)
          .setColorAttachmentFormat(renderData.drawImage.format);

  renderData.pipeline = pipelineBuilder.build();

  core.dispatch.destroyShaderModule(vertexShader, nullptr);
  core.dispatch.destroyShaderModule(fragmentShader, nullptr);

  vulkanDestroyer.addPipeline(renderData.pipeline);
  vulkanDestroyer.addPipelineLayout(renderData.pipelineLayout);
}

void Engine::drawImGUI(VkCommandBuffer cmd, VkImageView targetImage) {

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // UI
  ImGui::Begin("IMGUI BABY");
  ImGui::Text("slide:");
  ImGui::ColorEdit3("BGCOLOR", glm::value_ptr(renderData.backgroundColor));

  if (ImGui::CollapsingHeader("simulationData.planets")) {

    ImGui::DragFloat("planet scale", &simulationData.planetScale);
    ImGui::Text("Camera target: %d", simulationData.bodies[targetIndex].textureIndex);

    float logMin   = log10f(1e-9f);
    float logMax   = log10f(1);
    float logValue = log10f(simulationData.distanceScale);

    ImGui::SliderFloat("distance scale", &logValue, -9, 1, "%.10f");
    simulationData.distanceScale = powf(10.f, logValue);

    ImGui::DragFloat("global scale", &simulationData.globalScale);
    ImGui::SliderFloat("time scale", &simulationData.timeScale, 0.f, 1000.f);
    ImGui::SliderFloat("Render Scale", &renderData.renderScale, 0.1f, 1.f);

    for (int i = 0; i < simulationData.bodies.size(); i++) {

      if (ImGui::CollapsingHeader(std::to_string(i).c_str())) {
        std::string id = std::to_string(i);

        EntityData* entity = &simulationData.bodies[i].entityData;

        ImGui::InputFloat3(("position##" + id).c_str(), &entity->position.x);
        ImGui::InputFloat3(("scale##" + id).c_str(), &entity->scale.x);

        ImGui::ColorEdit4(
            ("color##" + id).c_str(),
            glm::value_ptr(simulationData.bodies[i].entityData.color));
        ImGui::SliderFloat(("tint##" + id).c_str(),
                           &simulationData.bodies[i].entityData.tint, 0.f, 1.f);
      }
    }
  }

  ImGui::End();
  ImGui::Render();

  VkRenderingAttachmentInfo colorAttachment = {};
  colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView   = targetImage;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
  colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

  VkRenderingInfo renderInfo = {};
  renderInfo.sType           = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderInfo.renderArea      = VkRect2D{VkOffset2D{0, 0}, swapchain.extent};
  renderInfo.layerCount      = 1;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments    = &colorAttachment;
  renderInfo.pDepthAttachment     = nullptr;
  renderInfo.pStencilAttachment   = nullptr;

  core.dispatch.cmdBeginRendering(cmd, &renderInfo);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  core.dispatch.cmdEndRendering(cmd);
}

void Engine::prepareFrame() {

  VK_ASSERT(core.dispatch.waitForFences(1, &getCurrentFrame().renderFence, true,
                                        1'000'000'000));

  // getCurrentFrame().descriptors.clearPools();

  VkAcquireNextImageInfoKHR acquireInfo = {};
  acquireInfo.sType      = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
  acquireInfo.swapchain  = swapchain.vkbSwapchain;
  acquireInfo.timeout    = 1'000'000'000; // 1 second
  acquireInfo.semaphore  = getCurrentFrame().imageAvailableSemaphore;
  acquireInfo.fence      = nullptr;
  acquireInfo.deviceMask = 0x1;

  VkResult result =
      core.dispatch.acquireNextImage2KHR(&acquireInfo, &swapchain.imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    swapchain.resize = true;
    return;
  }

  VK_ASSERT(core.dispatch.resetFences(1, &getCurrentFrame().renderFence));

  renderData.drawExtent.width =
      std::min(swapchain.extent.width, renderData.drawImage.extent.height) *
      renderData.renderScale;
  renderData.drawExtent.height =
      std::min(swapchain.extent.height, renderData.drawImage.extent.height) *
      renderData.renderScale;

  VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
  VK_ASSERT(core.dispatch.resetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_ASSERT(core.dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));
}

void Engine::submitFrame() {

  VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

  VK_ASSERT(core.dispatch.endCommandBuffer(cmd));

  // command is ready for submission
  VkCommandBufferSubmitInfo submitInfo = {};
  submitInfo.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  submitInfo.commandBuffer = cmd;

  VkSemaphoreSubmitInfo waitInfo = {};
  waitInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  waitInfo.semaphore             = getCurrentFrame().imageAvailableSemaphore;
  waitInfo.value                 = 0;
  waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;

  VkSemaphoreSubmitInfo signalInfo = {};
  signalInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  signalInfo.semaphore =
      renderData.renderFinishedSemaphores[swapchain.imageIndex];
  signalInfo.value     = 0;
  signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

  VkSubmitInfo2 submit            = {};
  submit.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit.waitSemaphoreInfoCount   = 1;
  submit.pWaitSemaphoreInfos      = &waitInfo;
  submit.commandBufferInfoCount   = 1;
  submit.pCommandBufferInfos      = &submitInfo;
  submit.signalSemaphoreInfoCount = 1;
  submit.pSignalSemaphoreInfos    = &signalInfo;

  VK_ASSERT(core.dispatch.queueSubmit2(core.graphicsQueue, 1, &submit,
                                       getCurrentFrame().renderFence));

  // prepare present
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pSwapchains      = &swapchain.vkbSwapchain.swapchain;
  presentInfo.swapchainCount   = 1;
  presentInfo.pWaitSemaphores =
      &renderData.renderFinishedSemaphores[swapchain.imageIndex];
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pImageIndices      = &swapchain.imageIndex;

  VkResult presentResult =
      core.dispatch.queuePresentKHR(core.graphicsQueue, &presentInfo);
  if (presentResult == VK_SUBOPTIMAL_KHR) {
    swapchain.resize = true;
  }

  renderData.frameNumber++;
}

void Engine::initDescriptors() {
  std::vector<DescriptorAllocator::PoolSizeRatio> frameSizes = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64},
  };

  VkDescriptorSetLayoutBinding bindings[2] = {};

  bindings[0].binding            = 0;
  bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount    = 1;
  bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
  bindings[0].pImmutableSamplers = nullptr;

  bindings[1].binding            = 1;
  bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[1].descriptorCount    = textureData.images.size();
  bindings[1].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[1].pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 2;
  layoutInfo.pBindings    = bindings;

  VK_ASSERT(core.dispatch.createDescriptorSetLayout(
      &layoutInfo, nullptr, &renderData.gpuSceneDataLayout));

  DescriptorWriter writer;
  writer.init(core.dispatch);

  for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
    renderData.frames[i].descriptors.init(core.dispatch, 1000, frameSizes);
    renderData.frames[i].descriptorSet =
        renderData.frames[i].descriptors.allocate(renderData.gpuSceneDataLayout,
                                                  nullptr);

    writer.writeBuffer(0, renderData.frames[i].gpuBuffer.buffer,
                       sizeof(GPUSceneData), 0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    writer.updateSet(renderData.frames[i].descriptorSet);

    std::vector<VkDescriptorImageInfo> imageInfos;
    for (size_t j = 0; j < textureData.images.size(); ++j) {
      imageInfos.push_back({
          .sampler     = textureData.samplerNearest,
          .imageView   = textureData.images[j].imageView,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      });
    }

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = renderData.frames[i].descriptorSet;
    write.dstBinding      = 1;
    write.dstArrayElement = 0;
    write.descriptorCount = (u32)imageInfos.size();
    write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo      = imageInfos.data();

    core.dispatch.updateDescriptorSets(1, &write, 0, nullptr);
  }
}

void Engine::draw() {

  prepareFrame();

  VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
  // wait for GPU to finish work

  leafUtil::transitionImage(cmd, renderData.drawImage.image,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  drawBackground(cmd);

  leafUtil::transitionImage(cmd, renderData.drawImage.image,
                            VK_IMAGE_LAYOUT_GENERAL,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  leafUtil::transitionImage(cmd, renderData.depthImage.image,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  drawGeometry(cmd);

  leafUtil::transitionImage(cmd, renderData.drawImage.image,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  leafUtil::transitionImage(cmd, swapchain.images[swapchain.imageIndex],
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  leafUtil::copyImageToImage(cmd, renderData.drawImage.image,
                             swapchain.images[swapchain.imageIndex],
                             renderData.drawExtent, swapchain.extent);

  leafUtil::transitionImage(cmd, swapchain.images[swapchain.imageIndex],
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  drawImGUI(cmd, swapchain.imageViews[swapchain.imageIndex]);
  leafUtil::transitionImage(cmd, swapchain.images[swapchain.imageIndex],
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  submitFrame();
}

void Engine::drawBackground(VkCommandBuffer cmd) {

  VkClearColorValue bg = {
      renderData.backgroundColor.r, renderData.backgroundColor.g,
      renderData.backgroundColor.b, renderData.backgroundColor.a};

  VkImageSubresourceRange clearRange =
      leafInit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

  core.dispatch.cmdClearColorImage(cmd, renderData.drawImage.image,
                                   VK_IMAGE_LAYOUT_GENERAL, &bg, 1,
                                   &clearRange);
}

void Engine::drawGeometry(VkCommandBuffer cmd) {
  VkRenderingAttachmentInfo colorAttachment = {};
  colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView   = renderData.drawImage.imageView;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
  colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue  = {};

  VkRenderingAttachmentInfo depthAttachment = {};
  depthAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depthAttachment.imageView   = renderData.depthImage.imageView;
  depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  depthAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.clearValue  = {.depthStencil = {1.0f, 0}};

  VkRenderingInfo renderInfo = {};
  renderInfo.sType           = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, renderData.drawExtent};
  renderInfo.layerCount = 1;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments    = &colorAttachment;
  renderInfo.pDepthAttachment     = &depthAttachment;
  renderInfo.pStencilAttachment   = nullptr;

  core.dispatch.cmdBeginRendering(cmd, &renderInfo);
  core.dispatch.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderData.pipeline);

  GPUSceneData* mapped =
      (GPUSceneData*)getCurrentFrame().gpuBuffer.allocation->GetMappedData();
  mapped->view       = camera.getViewMatrix();
  mapped->projection = camera.getProjectionMatrix();

  VkViewport viewport = {};

  viewport.x        = 0;
  viewport.y        = 0;
  viewport.width    = renderData.drawExtent.width;
  viewport.height   = renderData.drawExtent.height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;
  core.dispatch.cmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor      = {};
  scissor.offset.x      = 0;
  scissor.offset.y      = 0;
  scissor.extent.width  = renderData.drawExtent.width;
  scissor.extent.height = renderData.drawExtent.height;
  core.dispatch.cmdSetScissor(cmd, 0, 1, &scissor);

  for (size_t i = 0; i < simulationData.bodies.size(); i++) {

    core.dispatch.cmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipelineLayout, 0, 1,
        &getCurrentFrame().descriptorSet, 0, nullptr);

    EntityData* entity = &simulationData.bodies[i].entityData;

    VertPushData mesh        = {};
    mesh.model               = entity->model;
    mesh.color               = entity->color;
    mesh.blendFactor         = entity->tint;
    mesh.vertexBufferAddress = entity->mesh.meshBuffers.vertexBufferAddress;
    mesh.textureIndex        = simulationData.bodies[i].textureIndex;

    VkPushConstantsInfo vertPushInfo = {};
    vertPushInfo.sType               = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
    vertPushInfo.layout              = renderData.pipelineLayout;
    vertPushInfo.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
    vertPushInfo.offset              = 0;
    vertPushInfo.size                = sizeof(VertPushData);
    vertPushInfo.pValues             = &mesh;

    core.dispatch.cmdPushConstants(cmd, vertPushInfo.layout,
                                   vertPushInfo.stageFlags, vertPushInfo.offset,
                                   vertPushInfo.size, vertPushInfo.pValues);

    core.dispatch.cmdBindIndexBuffer(cmd, entity->mesh.getIndexBuffer(), 0,
                                     VK_INDEX_TYPE_UINT32);

    core.dispatch.cmdDrawIndexed(cmd, entity->mesh.surfaces[0].count, 1,
                                 entity->mesh.surfaces[0].startIndex, 0, 0);
  }

  core.dispatch.cmdEndRendering(cmd);
}

AllocatedBuffer Engine::createBuffer(size_t allocSize, VkBufferUsageFlags usage,
                                     VmaMemoryUsage memoryUsage) {

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size               = allocSize;
  bufferInfo.usage              = usage;

  VmaAllocationCreateInfo vmaAllocInfo = {};
  vmaAllocInfo.usage                   = memoryUsage;
  vmaAllocInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  AllocatedBuffer newBuffer;

  VK_ASSERT(vmaCreateBuffer(core.allocator, &bufferInfo, &vmaAllocInfo,
                            &newBuffer.buffer, &newBuffer.allocation,
                            &newBuffer.allocationInfo));

  vulkanDestroyer.addAllocatedBuffer(newBuffer);
  return newBuffer;
}

AllocatedImage Engine::createImage(VkExtent3D extent, VkFormat format,
                                   VkImageUsageFlags usage, bool mipmapped) {
  AllocatedImage newImage;
  newImage.format = format;
  newImage.extent = extent;

  VkImageCreateInfo imageInfo =
      leafInit::imageCreateInfo(format, usage, extent);
  if (mipmapped) {
    imageInfo.mipLevels =
        (u32)std::floor(std::log2(std::max(extent.width, extent.height))) + 1;
  }

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
  allocInfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VK_ASSERT(vmaCreateImage(core.allocator, &imageInfo, &allocInfo,
                           &newImage.image, &newImage.allocation, nullptr));

  VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
  if (format == VK_FORMAT_D32_SFLOAT) {
    aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
  }

  VkImageViewCreateInfo viewInfo =
      leafInit::imageViewCreateInfo(format, newImage.image, aspectFlag);
  viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;

  VK_ASSERT(
      core.dispatch.createImageView(&viewInfo, nullptr, &newImage.imageView));

  vulkanDestroyer.addImage(newImage);

  return newImage;
}

AllocatedImage Engine::createImage(void* data, VkExtent3D extent,
                                   VkFormat format, VkImageUsageFlags usage,
                                   bool mipmapped) {

  VK_ASSERT(core.dispatch.resetFences(1, &immediateData.fence));
  VK_ASSERT(core.dispatch.resetCommandBuffer(immediateData.commandBuffer, 0));

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags                   = 0;
  poolInfo.queueFamilyIndex        = core.graphicsQueueFamily;
  poolInfo.pNext                   = nullptr;

  core.dispatch.createCommandPool(&poolInfo, nullptr,
                                  &immediateData.commandPool);

  VkCommandBuffer cmd = immediateData.commandBuffer;

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandBufferCount = 1;
  allocInfo.commandPool        = immediateData.commandPool;

  core.dispatch.allocateCommandBuffers(&allocInfo, &cmd);

  u32 dataSize = extent.depth * extent.width * extent.height * 4;

  AllocatedBuffer uploadBuffer = createBuffer(
      dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  memcpy(uploadBuffer.allocationInfo.pMappedData, data, dataSize);

  AllocatedImage newImage = createImage(
      extent, format,
      usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      mipmapped);

  VkCommandBufferBeginInfo beginInfo = leafInit::commandBufferBeginInfo(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  core.dispatch.beginCommandBuffer(cmd, &beginInfo);

  leafUtil::transitionImage(cmd, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkBufferImageCopy copyRegion = {};
  copyRegion.bufferOffset      = 0;
  copyRegion.bufferRowLength   = 0;
  copyRegion.bufferImageHeight = 0;

  copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  copyRegion.imageSubresource.mipLevel       = 0;
  copyRegion.imageSubresource.baseArrayLayer = 0;
  copyRegion.imageSubresource.layerCount     = 1;
  copyRegion.imageExtent                     = extent;

  // copy the buffer into the image
  vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, newImage.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  leafUtil::transitionImage(cmd, newImage.image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  core.dispatch.endCommandBuffer(cmd);

  VkCommandBufferSubmitInfo cmdSubmitInfo = {};
  cmdSubmitInfo.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdSubmitInfo.commandBuffer = cmd;
  cmdSubmitInfo.deviceMask    = 0;

  VkSubmitInfo2 submit          = {};
  submit.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos    = &cmdSubmitInfo;

  VK_ASSERT(core.dispatch.queueSubmit2(core.graphicsQueue, 1, &submit,
                                       immediateData.fence));
  VK_ASSERT(
      core.dispatch.waitForFences(1, &immediateData.fence, true, U64_MAX));

  // destroyBuffer(uploadBuffer);

  vulkanDestroyer.addImage(newImage);
  return newImage;
}
GPUMeshBuffers Engine::uploadMesh(std::span<u32>    indices,
                                  std::span<Vertex> vertices) {
  const size_t   vertexBufferSize = vertices.size() * sizeof(Vertex);
  const size_t   indexBufferSize  = indices.size() * sizeof(u32);
  GPUMeshBuffers newSurface;

  newSurface.vertexBuffer = createBuffer(
      vertexBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo deviceAddressInfo = {};
  deviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  deviceAddressInfo.buffer = newSurface.vertexBuffer.buffer;
  newSurface.vertexBufferAddress =
      core.dispatch.getBufferDeviceAddress(&deviceAddressInfo);

  newSurface.indexBuffer = createBuffer(indexBufferSize,
                                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                        VMA_MEMORY_USAGE_GPU_ONLY);

  AllocatedBuffer staging =
      createBuffer(vertexBufferSize + indexBufferSize,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  auto data = static_cast<u8*>(staging.allocation->GetMappedData());
  memcpy(data, vertices.data(), vertexBufferSize);
  memcpy(data + vertexBufferSize, indices.data(), indexBufferSize);

  // immediate submit
  VK_ASSERT(core.dispatch.resetFences(1, &immediateData.fence));
  VK_ASSERT(core.dispatch.resetCommandBuffer(immediateData.commandBuffer, 0));

  VkCommandBuffer cmd = immediateData.commandBuffer;

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VK_ASSERT(core.dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));

  VkBufferCopy2 vertexCopy = {};
  vertexCopy.sType         = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
  vertexCopy.size          = vertexBufferSize;

  VkCopyBufferInfo2 vertexCopyInfo = {};
  vertexCopyInfo.sType             = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
  vertexCopyInfo.srcBuffer         = staging.buffer;
  vertexCopyInfo.dstBuffer         = newSurface.vertexBuffer.buffer;
  vertexCopyInfo.regionCount       = 1;
  vertexCopyInfo.pRegions          = &vertexCopy;

  core.dispatch.cmdCopyBuffer2(cmd, &vertexCopyInfo);

  VkBufferCopy2 indexCopy = {};
  indexCopy.sType         = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
  indexCopy.srcOffset     = vertexBufferSize;
  indexCopy.size          = indexBufferSize;

  VkCopyBufferInfo2 indexCopyInfo = {};
  indexCopyInfo.sType             = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
  indexCopyInfo.srcBuffer         = staging.buffer;
  indexCopyInfo.dstBuffer         = newSurface.indexBuffer.buffer;
  indexCopyInfo.regionCount       = 1;
  indexCopyInfo.pRegions          = &indexCopy;

  core.dispatch.cmdCopyBuffer2(cmd, &indexCopyInfo);

  VK_ASSERT(core.dispatch.endCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmdSubmitInfo = {};
  cmdSubmitInfo.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdSubmitInfo.commandBuffer = cmd;

  VkSubmitInfo2 submit          = {};
  submit.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos    = &cmdSubmitInfo;

  VK_ASSERT(core.dispatch.queueSubmit2(core.graphicsQueue, 1, &submit,
                                       immediateData.fence));
  VK_ASSERT(
      core.dispatch.waitForFences(1, &immediateData.fence, true, U64_MAX));

  vmaDestroyBuffer(core.allocator, staging.buffer, staging.allocation);

  return newSurface;
}

void Engine::initSolarSystem() {

  f64 AU = 1.496e11;

  Body sun         = {};
  sun.textureIndex = textureData.imageMap["sun"];
  sun.baseScale    = 109; // radius relative to earth
  sun.isPlanet     = false;
  simulationData.bodies.push_back(sun);

  Body mercury         = {};
  mercury.textureIndex = textureData.imageMap["mercury"];
  mercury.isPlanet     = true;
  mercury.baseScale    = 0.3829;
  mercury.a            = 0.387098 * AU;
  mercury.e            = 0.205630;
  mercury.T            = 87.9691;
  mercury.M0           = 174.796;
  mercury.t            = 0;
  mercury.o            = 29.124;
  mercury.Omega        = 48.331;
  mercury.i            = 3.38;
  simulationData.bodies.push_back(mercury);

  Body venus         = {};
  venus.textureIndex = textureData.imageMap["venus"];
  venus.isPlanet     = true;
  venus.baseScale    = 0.9499;        // radius in earths
  venus.a            = 0.723332 * AU; // semi-major axis
  venus.e            = 0.0167086;     // eccentricity
  venus.T            = 365.256363004; // days
  venus.M0           = 6.38122045;    // radians
  venus.t            = 0;             // orbital position (days)
  venus.o            = 54.884;        // argument of perihelion
  venus.Omega        = 76.680;        // longitute of ascending node
  venus.i            = 3.86;          // inclination to sun's equator

  simulationData.bodies.push_back(venus);

  Body earth         = {};
  earth.textureIndex = textureData.imageMap["earth"];
  earth.isPlanet     = true;
  earth.baseScale    = 1;
  earth.a            = 1 * AU;
  earth.e            = 0.0167086;
  earth.T            = 365.256363004;
  earth.M0           = 358.617;
  earth.t            = 0;
  earth.o            = 114.20783;
  earth.Omega        = -11.26064;
  earth.i            = 7.155;
  simulationData.bodies.push_back(earth);

  Body mars         = {};
  mars.textureIndex = textureData.imageMap["mars"];
  mars.isPlanet     = true;
  mars.baseScale    = 0.533;
  mars.a            = 1.523680 * AU;
  mars.e            = 0.0934;
  mars.T            = 686.980;
  mars.M0           = 19.412;
  mars.t            = 0;
  mars.o            = 286.5;
  mars.Omega        = 49.57854;
  mars.i            = 3.86;
  simulationData.bodies.push_back(mars);

  Body saturn         = {};
  saturn.textureIndex = textureData.imageMap["saturn"];
  saturn.isPlanet     = true;
  saturn.baseScale    = 9.449;
  saturn.a            = 9.5826 * AU;
  saturn.e            = 0.0565;
  saturn.T            = 10'755.70;
  saturn.M0           = 317.020;
  saturn.t            = 0;
  saturn.o            = 339.392;
  saturn.Omega        = 113.665;
  saturn.i            = 5.51;
  simulationData.bodies.push_back(saturn);

  Body jupiter         = {};
  jupiter.textureIndex = textureData.imageMap["jupiter"];
  jupiter.isPlanet     = true;
  jupiter.baseScale    = 11.209;
  jupiter.a            = 5.2038 * AU;
  jupiter.e            = 0.0489;
  jupiter.T            = 4'332.59;
  jupiter.M0           = 20.020;
  jupiter.t            = 0;
  jupiter.o            = 273.867;
  jupiter.Omega        = 100.464;
  jupiter.i            = 6.09;
  simulationData.bodies.push_back(jupiter);

  Body uranus         = {};
  uranus.textureIndex = textureData.imageMap["uranus"];
  uranus.isPlanet     = true;
  uranus.baseScale    = 4.007;
  uranus.a            = 19.19126 * AU;
  uranus.e            = 0.04717;
  uranus.T            = 30'688.5;
  uranus.M0           = 142.238600;
  uranus.t            = 0;
  uranus.o            = 96.998857;
  uranus.Omega        = 74.006;
  uranus.i            = 6.48;
  simulationData.bodies.push_back(uranus);

  Body neptune         = {};
  neptune.textureIndex = textureData.imageMap["neptune"];
  neptune.isPlanet     = true;
  neptune.baseScale    = 3.883;
  neptune.a            = 30.07 * AU;
  neptune.e            = 0.008678;
  neptune.T            = 60'195;
  neptune.M0           = 142.238600;
  neptune.t            = 0;
  neptune.o            = 273.187;
  neptune.Omega        = 131.783;
  neptune.i            = 6.43;
  simulationData.bodies.push_back(neptune);

  MeshAsset planetAsset = leafGltf::loadGltfMesh("assets/planet.gltf");

  GPUMeshBuffers planetMesh =
      uploadMesh(planetAsset.indices, planetAsset.vertices);

  for (u32 i = 0; i < simulationData.bodies.size(); i++) {
    EntityData* entity       = &simulationData.bodies[i].entityData;
    entity->mesh             = planetAsset;
    entity->mesh.meshBuffers = planetMesh;
    entity->position =
        simulationData.bodies[i].getPosition(simulationData.distanceScale);
    entity->scale = glm::vec3(1);
  }
  // ugly, i know
  sun.entityData.position = glm::vec3(0);
}

void Engine::processEvent(SDL_Event& e) {

  ImGui_ImplSDL3_ProcessEvent(&e);

  if (e.type == SDL_EVENT_KEY_DOWN) {
    if (e.key.scancode == SDL_SCANCODE_ESCAPE) {
      surface.captureMouse = !surface.captureMouse;
      camera.active        = !camera.active;
      SDL_SetWindowRelativeMouseMode(surface.window, surface.captureMouse);
    }
    if (e.key.scancode == SDL_SCANCODE_Q) {
      if(targetIndex <= 0) {
        targetIndex = simulationData.bodies.size() - 1;
      } else  {
        targetIndex--;
      }

      camera.setTarget(simulationData.bodies[targetIndex]);
    }

    if (e.key.scancode == SDL_SCANCODE_E) {

      if(targetIndex >= simulationData.bodies.size() - 1) {
        targetIndex = 0;
      } else {
        targetIndex++;
      }
      camera.setTarget(simulationData.bodies[targetIndex]);
    }
    if (e.key.scancode == SDL_SCANCODE_TAB) {
      camera.noTarget();


    }
  }

  camera.handleMouse(e);
  camera.handleInput();
}

void Engine::update(f64 dt) {

  for (u32 i = 0; i < simulationData.bodies.size(); i++) {

    Body* body = &simulationData.bodies[i];

    f32 scale = body->baseScale * simulationData.globalScale;
    if (body->isPlanet) {
      scale *= simulationData.planetScale;
    }
    body->update(simulationData.timeScale * dt);
    body->entityData.position = body->getPosition(simulationData.distanceScale);

    glm::mat4 model = glm::mat4(1.f);
    model           = glm::translate(model, body->entityData.position);
    model = glm::rotate(model, body->entityData.rotation.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, body->entityData.rotation.y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, body->entityData.rotation.z, glm::vec3(0, 0, 1));
    model = glm::scale(model, glm::vec3(scale) * body->entityData.scale);

    body->entityData.model = model;
  }

  int width, height;
  SDL_GetWindowSizeInPixels(surface.window, &width, &height);
  if (width != swapchain.extent.width || height != swapchain.extent.height) {
    swapchain.resize = true;
    resizeSwapchain(width, height);
  }

  camera.update(dt);
}

void Engine::initSceneData() {
  camera = Camera{};

  camera.aspectRatio = swapchain.extent.width / (f32)swapchain.extent.height;
  for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {

    renderData.frames[i].gpuBuffer =
        createBuffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VMA_MEMORY_USAGE_CPU_TO_GPU);
  }

  // checkerboard image
  uint32_t black   = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
  uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
  std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
    }
  }

  std::filesystem::path path = "assets/img/";

  u32 index = 0;
  for (const auto& entry : std::filesystem::directory_iterator(path)) {

    int width, height, channels;
    if (entry.is_regular_file()) {

      unsigned char* data =
          stbi_load(entry.path().c_str(), &width, &height, &channels, 4);
      AllocatedImage image =
          createImage(data, VkExtent3D{(u32)width, (u32)height, 1},
                      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

      textureData.images.push_back(image);
      textureData.imageMap.insert({entry.path().stem().c_str(), index});
      ++index;

      fmt::println("loaded {}", entry.path().stem().c_str());
      vulkanDestroyer.addImage(image);
    }
  }

  textureData.errorImage =
      createImage(pixels.data(), VkExtent3D{16, 16, 1},
                  VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

  sampl.magFilter = VK_FILTER_NEAREST;
  sampl.minFilter = VK_FILTER_NEAREST;

  core.dispatch.createSampler(&sampl, nullptr, &textureData.samplerNearest);

  sampl.magFilter = VK_FILTER_LINEAR;
  sampl.minFilter = VK_FILTER_LINEAR;
  core.dispatch.createSampler(&sampl, nullptr, &textureData.samplerLinear);

  vulkanDestroyer.addImage(textureData.errorImage);
  // vulkanDestroyer.addSampler(textureData.samplerLinear);
  // vulkanDestroyer.addSampler(textureData.samplerNearest);
}

void Engine::resizeSwapchain(u32 width, u32 height) {
  core.dispatch.deviceWaitIdle();

  vkb::destroy_swapchain(swapchain.vkbSwapchain);

  surface.extent = {width, height};
  initSwapchain();
  swapchain.resize = false;
}
}; // namespace LeafEngine
