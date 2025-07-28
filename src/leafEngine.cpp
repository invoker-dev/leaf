// Actual todo
// TODO :: glTF loading
// TODO :: fix the pipelines for multiple shaders

// TODO: Stencil Testing
// TODO: Blending / transparency
// TODO: mipmapping, texturesampling
// TODO: Lighting
// TODO: Normal maps
// TODO: Various Post processing
// CONTINOUS TODO: Real Architecture
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fmt/base.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>
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
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
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
  initCamera();

  initDescriptors();
  initImGUI();
  initPipeline();
  initEntities();
};
Engine::~Engine() {

  vkAssert(core.dispatch.deviceWaitIdle());

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
      // .set_debug_messenger_severity(DEBUG_SEVERITY)
      // .set_debug_messenger_type(DEBUG_TYPE)
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

  vkAssert(core.dispatch.createImageView(&renderImageViewCreateInfo, nullptr,
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

    vkAssert(core.dispatch.createCommandPool(
        &cmdPoolInfo, nullptr, &renderData.frames[i].commandPool));

    vulkanDestroyer.addCommandPool(renderData.frames[i].commandPool);

    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = renderData.frames[i].commandPool;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAssert(core.dispatch.allocateCommandBuffers(
        &cmdAllocInfo, &renderData.frames[i].commandBuffer));
  }
  // immediate
  vkAssert(core.dispatch.createCommandPool(&cmdPoolInfo, nullptr,
                                           &immediateData.commandPool));

  VkCommandBufferAllocateInfo immCmdAllocInfo = {};
  immCmdAllocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  immCmdAllocInfo.commandPool = immediateData.commandPool;
  immCmdAllocInfo.commandBufferCount = 1;
  immCmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  vkAssert(core.dispatch.allocateCommandBuffers(&immCmdAllocInfo,
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
    vkAssert(core.dispatch.createFence(&fenceInfo, nullptr,
                                       &renderData.frames[i].renderFence));
    vulkanDestroyer.addFence(renderData.frames[i].renderFence);

    vkAssert(core.dispatch.createSemaphore(
        &semaphoreInfo, nullptr,
        &renderData.frames[i].imageAvailableSemaphore));

    vulkanDestroyer.addSemaphore(renderData.frames[i].imageAvailableSemaphore);
  }

  for (size_t i = 0; i < imageCount; i++) {

    vkAssert(core.dispatch.createSemaphore(
        &semaphoreInfo, nullptr, &renderData.renderFinishedSemaphores[i]));

    vulkanDestroyer.addSemaphore(renderData.renderFinishedSemaphores[i]);
  }

  vkAssert(
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

  vkAssert(core.dispatch.createDescriptorPool(&poolInfo, nullptr,
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
  initInfo.MinImageCount               = 3;
  initInfo.ImageCount                  = 3;
  initInfo.UseDynamicRendering         = true;
  initInfo.PipelineRenderingCreateInfo = {};
  initInfo.PipelineRenderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats =
      &swapchain.imageFormat;
  initInfo.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
  initInfo.Allocator       = nullptr;
  initInfo.CheckVkResultFn = vkAssert;

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

  VkPushConstantRange pushConstantRanges[2];

  pushConstantRanges[0].offset     = 0;
  pushConstantRanges[0].size       = sizeof(VertPushData);
  pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  // align by 16 bytes
  constexpr size_t fragOffset      = (sizeof(VertPushData) + 15) & ~15;
  pushConstantRanges[1].offset     = fragOffset;
  pushConstantRanges[1].size       = sizeof(FragPushData);
  pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.flags = 0;
  pipelineLayoutInfo.setLayoutCount =
      renderData.descriptorAllocator.descriptorSetLayouts.size();
  pipelineLayoutInfo.pSetLayouts =
      renderData.descriptorAllocator.descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 2;
  pipelineLayoutInfo.pPushConstantRanges    = pushConstantRanges;

  vkAssert(core.dispatch.createPipelineLayout(&pipelineLayoutInfo, nullptr,
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
          .disableBlending()
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
  ImGui::SliderFloat("Render Scale", &renderData.renderScale, 0.1, 1.f);

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

  vkAssert(core.dispatch.waitForFences(1, &getCurrentFrame().renderFence, true,
                                       1'000'000'000));

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

  vkAssert(core.dispatch.resetFences(1, &getCurrentFrame().renderFence));

  renderData.drawExtent.width =
      std::min(swapchain.extent.width, renderData.drawImage.extent.height) *
      renderData.renderScale;
  renderData.drawExtent.height =
      std::min(swapchain.extent.height, renderData.drawImage.extent.height) *
      renderData.renderScale;

  VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
  vkAssert(core.dispatch.resetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkAssert(core.dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));
}

void Engine::submitFrame() {

  VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

  vkAssert(core.dispatch.endCommandBuffer(cmd));

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

  vkAssert(core.dispatch.queueSubmit2(core.graphicsQueue, 1, &submit,
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
  auto* descAllocator            = &renderData.descriptorAllocator;
  renderData.descriptorAllocator = {core.dispatch};
  renderData.descriptorAllocator.setupLayouts();
  renderData.descriptorAllocator.setupPools();

  for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descAllocator->descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = descAllocator->descriptorSetLayouts.data();

    vkAssert(core.dispatch.allocateDescriptorSets(
        &allocInfo, &descAllocator->descriptorSets[i]));

    VkDescriptorBufferInfo cameraBufferInfo{};
    cameraBufferInfo.buffer = renderData.cameraBuffers[i].buffer;
    cameraBufferInfo.offset = 0;
    cameraBufferInfo.range  = sizeof(CameraUBO);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = descAllocator->descriptorSets[i];
    descriptorWrite.dstBinding      = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.pBufferInfo     = &cameraBufferInfo;

    core.dispatch.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
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
  colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
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

  CameraUBO* mapped =
      (CameraUBO*)renderData
          .cameraBuffers[renderData.frameNumber % FRAMES_IN_FLIGHT]
          .allocation->GetMappedData();
  mapped->view       = camera.getViewMatrix();
  mapped->projection = camera.getProjectionMatrix();

  core.dispatch.cmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipelineLayout, 0, 1,
      &renderData.descriptorAllocator
           .descriptorSets[renderData.frameNumber % FRAMES_IN_FLIGHT],
      0, nullptr);

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

  for (size_t i = 0; i < entities.size(); i++) {


    VertPushData mesh        = {};
    mesh.model               = entities[i].model;
    mesh.color               = {};
    mesh.vertexBufferAddress = entities[i].mesh.meshBuffers.vertexBufferAddress;

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

    core.dispatch.cmdBindIndexBuffer(cmd, entities[i].mesh.getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    core.dispatch.cmdDrawIndexed(cmd, entities[i].mesh.surfaces[0].count, 1,
                                 entities[i].mesh.surfaces[0].startIndex, 0, 0);
  }

  core.dispatch.cmdEndRendering(cmd);
}

AllocatedBuffer Engine::allocateBuffer(size_t             allocSize,
                                       VkBufferUsageFlags usage,
                                       VmaMemoryUsage     memoryUsage) {

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size               = allocSize;
  bufferInfo.usage              = usage;

  VmaAllocationCreateInfo vmaAllocInfo = {};
  vmaAllocInfo.usage                   = memoryUsage;
  vmaAllocInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  AllocatedBuffer newBuffer;

  vkAssert(vmaCreateBuffer(core.allocator, &bufferInfo, &vmaAllocInfo,
                           &newBuffer.buffer, &newBuffer.allocation,
                           &newBuffer.allocationInfo));

  vulkanDestroyer.addAllocatedBuffer(newBuffer);
  return newBuffer;
}

GPUMeshBuffers Engine::uploadMesh(std::span<u32>    indices,
                                  std::span<Vertex> vertices) {
  const size_t   vertexBufferSize = vertices.size() * sizeof(Vertex);
  const size_t   indexBufferSize  = indices.size() * sizeof(u32);
  GPUMeshBuffers newSurface;

  newSurface.vertexBuffer = allocateBuffer(
      vertexBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo deviceAddressInfo = {};
  deviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  deviceAddressInfo.buffer = newSurface.vertexBuffer.buffer;
  newSurface.vertexBufferAddress =
      core.dispatch.getBufferDeviceAddress(&deviceAddressInfo);

  newSurface.indexBuffer = allocateBuffer(indexBufferSize,
                                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          VMA_MEMORY_USAGE_GPU_ONLY);

  AllocatedBuffer staging = allocateBuffer(vertexBufferSize + indexBufferSize,
                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_MEMORY_USAGE_CPU_ONLY);

  auto data = static_cast<u8*>(staging.allocation->GetMappedData());
  memcpy(data, vertices.data(), vertexBufferSize);
  memcpy(data + vertexBufferSize, indices.data(), indexBufferSize);

  // immediate submit
  vkAssert(core.dispatch.resetFences(1, &immediateData.fence));
  vkAssert(core.dispatch.resetCommandBuffer(immediateData.commandBuffer, 0));

  VkCommandBuffer cmd = immediateData.commandBuffer;

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkAssert(core.dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));

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

  vkAssert(core.dispatch.endCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmdSubmitInfo = {};
  cmdSubmitInfo.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdSubmitInfo.commandBuffer = cmd;

  VkSubmitInfo2 submit          = {};
  submit.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos    = &cmdSubmitInfo;

  vkAssert(core.dispatch.queueSubmit2(core.graphicsQueue, 1, &submit,
                                      immediateData.fence));
  vkAssert(core.dispatch.waitForFences(1, &immediateData.fence, true, U64_MAX));

  vmaDestroyBuffer(core.allocator, staging.buffer, staging.allocation);

  return newSurface;
}

void Engine::initEntities() {

  // cubeSystem.mesh.meshBuffers =
  //     uploadMesh(cubeSystem.mesh.indices, cubeSystem.mesh.vertices);

  MeshAsset meshAsset = leafGltf::loadGltfMesh("assets/planet.gltf");

  GPUMeshBuffers gltfMesh = uploadMesh(meshAsset.indices, meshAsset.vertices);

  for (u32 i = 0; i < 1; i++) {
    Entity entity;
    entity.position         = glm::vec3(0);
    entity.rotation         = glm::vec3(0);
    entity.scale            = glm::vec3(1);
    entity.mesh             = meshAsset;
    entity.mesh.meshBuffers = gltfMesh;

    glm::mat4 model = glm::mat4(1.f);

    model = glm::scale(model, entity.scale);
    model = glm::rotate(model, entity.rotation.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, entity.rotation.y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, entity.rotation.z, glm::vec3(0, 0, 1));
    model = glm::translate(model, entity.position);

    entity.model = model;
    entities.push_back(entity);
  }
}

void Engine::processEvent(SDL_Event& e) {

  ImGui_ImplSDL3_ProcessEvent(&e);

  if (e.type == SDL_EVENT_KEY_DOWN) {
    if (e.key.scancode == SDL_SCANCODE_ESCAPE) {
      surface.captureMouse = !surface.captureMouse;
      camera.active        = !camera.active;
      SDL_SetWindowRelativeMouseMode(surface.window, surface.captureMouse);
    }
  }

  camera.handleMouse(e);
  camera.handleInput();
}

void Engine::update(f64 dt) {

  int width, height;
  SDL_GetWindowSizeInPixels(surface.window, &width, &height);
  if (width != swapchain.extent.width || height != swapchain.extent.height) {
    swapchain.resize = true;
    resizeSwapchain(width, height);
  }

  camera.update(dt);
}

void Engine::initCamera() {
  camera        = Camera{};
  camera.frames = FRAMES_IN_FLIGHT;

  camera.aspectRatio = swapchain.extent.width / (f32)swapchain.extent.height;
  renderData.cameraBuffers = std::vector<AllocatedBuffer>(FRAMES_IN_FLIGHT);
  for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
    renderData.cameraBuffers[i] =
        allocateBuffer(sizeof(CameraUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VMA_MEMORY_USAGE_CPU_TO_GPU);
  }
}

void Engine::resizeSwapchain(u32 width, u32 height) {
  core.dispatch.deviceWaitIdle();

  vkb::destroy_swapchain(swapchain.vkbSwapchain);

  surface.extent = {width, height};
  initSwapchain();
  swapchain.resize = false;
}
}; // namespace LeafEngine
