#include "fastgltf/types.hpp"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fmt/base.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <iterator>
#include <leafEngine.h>
#include <leafInit.h>
#include <leafStructs.h>
#include <leafUtil.h>
#include <pipelineBuilder.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

LeafEngine::LeafEngine() {

  createSDLWindow();
  initVulkan();
  getQueues();
  initSwapchain();
  initCommands();
  initSynchronization();
  initCamera();
  initDescriptorLayout();
  initDescriptorPool();
  initDescriptorSets();
  initImGUI();
  initPipeline();
  initMesh();
}
LeafEngine::~LeafEngine() {

  vkAssert(dispatch.deviceWaitIdle());

  // destroys primitives (images, buffers, fences etc)
  vulkanDestroyer.flush(dispatch, allocator);

  vmaDestroyAllocator(allocator);
  ImGui_ImplVulkan_Shutdown();
  vkb::destroy_swapchain(swapchain);
  vkb::destroy_device(device);
  vkb::destroy_surface(instance, surface);
  vkb::destroy_instance(instance);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void LeafEngine::createSDLWindow() {

  SDL_WindowFlags sdlFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

  window = SDL_CreateWindow("LeafEngine", 2000, 1500, sdlFlags);
  if (!window) {
    fmt::print("failed to init SDL window: {}\n", SDL_GetError());
    std::exit(-1);
  }

  if (!SDL_ShowWindow(window)) {
    fmt::print("failed to show SDL window: {}\n", SDL_GetError());
    std::exit(-1);
  }
}

void LeafEngine::initVulkan() {

  unsigned int count = 0;
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
      .request_validation_layers(useValidationLayers)
      .enable_extensions(sdlExtensions);

  auto returnedInstance = builder.build();

  if (!returnedInstance) {
    fmt::println("Failed to build vkbInstance: {}",
                 returnedInstance.error().message());
    std::exit(-1);
  }

  instance = returnedInstance.value();

  instanceDispatchTable = instance.make_table();
  bool result =
      SDL_Vulkan_CreateSurface(window, instance.instance, nullptr, &surface);
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

  vkb::PhysicalDeviceSelector selector{instance};
  auto returnedPhysicalDevice = selector.set_minimum_version(1, 3)
                                    .set_required_features_13(features13)
                                    .set_required_features_12(features12)
                                    .set_surface(surface)
                                    .select();

  if (!returnedPhysicalDevice) {
    fmt::println("Failed to find physical device: {}",
                 returnedPhysicalDevice.error().message());
    std::exit(-1);
  }

  physicalDevice = returnedPhysicalDevice.value();

  vkb::DeviceBuilder deviceBuilder{physicalDevice};
  auto               returnedDevice = deviceBuilder.build();

  if (!returnedDevice) {
    fmt::println("Failed to build device: {}",
                 returnedDevice.error().message());
    std::exit(-1);
  }

  device   = returnedDevice.value();
  dispatch = device.make_table();

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice         = physicalDevice;
  allocatorInfo.device                 = device;
  allocatorInfo.instance               = instance;
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&allocatorInfo, &allocator);
}

void LeafEngine::getQueues() {
  auto gq = device.get_queue(vkb::QueueType::graphics);
  if (!gq.has_value()) {
    fmt::println("failed to get graphics queue: {}", gq.error().message());
    std::exit(-1);
  }
  graphicsQueue = gq.value();
  graphicsQueueFamily =
      device.get_queue_index(vkb::QueueType::graphics).value();
}

void LeafEngine::initSwapchain() {

  int width, height;
  SDL_GetWindowSizeInPixels(window, &width, &height);
  windowExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

  fmt::println("Window size:   [{} {}]", width, height);

  vkb::SwapchainBuilder swapchain_builder{physicalDevice, device, surface};
  auto                  returnedSwapchain =
      swapchain_builder.set_desired_format(surfaceFormat)
          .set_desired_format(VkSurfaceFormatKHR{
              .format     = swapchainImageFormat,
              .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .set_desired_min_image_count(framesInFlight)
          .set_desired_extent(width, height)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build();

  if (!returnedSwapchain) {
    fmt::println("failed to build swapchain: {}",
                 returnedSwapchain.error().message());
    std::exit(-1);
  }
  VkFormat actualFormat = returnedSwapchain.value().image_format;
  printf("Actual swapchain format: %d\n", (int)actualFormat);
  printf("Variable says: %d\n", (int)swapchainImageFormat);

  vkb::destroy_swapchain(swapchain);
  swapchain           = returnedSwapchain.value();
  swapchainImageViews = swapchain.get_image_views().value();
  swapchainImages     = swapchain.get_images().value();
  swapchainExtent     = swapchain.extent;

  VkExtent3D drawImageExtent = {};
  drawImageExtent.width      = windowExtent.width;
  drawImageExtent.height     = windowExtent.height;
  drawImageExtent.depth      = 1;

  drawImage             = {};
  drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
  ;
  drawImage.imageExtent = drawImageExtent;

  VkImageUsageFlags drawImageUsages = {};
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo renderImageCreateInfo = leafInit::imageCreateInfo(
      drawImage.imageFormat, drawImageUsages, drawImageExtent);

  VmaAllocationCreateInfo renderImageAllocationInfo = {};
  renderImageAllocationInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
  renderImageAllocationInfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vmaCreateImage(allocator, &renderImageCreateInfo, &renderImageAllocationInfo,
                 &drawImage.image, &drawImage.allocation, nullptr);

  VkImageViewCreateInfo renderImageViewCreateInfo =
      leafInit::imageViewCreateInfo(drawImage.imageFormat, drawImage.image,
                                    VK_IMAGE_ASPECT_COLOR_BIT);

  vkAssert(dispatch.createImageView(&renderImageViewCreateInfo, nullptr,
                                    &drawImage.imageView));
}

void LeafEngine::initCommands() {

  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cmdPoolInfo.queueFamilyIndex = graphicsQueueFamily;

  for (size_t i = 0; i < framesInFlight; i++) {

    vkAssert(dispatch.createCommandPool(&cmdPoolInfo, nullptr,
                                        &frames[i].commandPool));

    vulkanDestroyer.addCommandPool(frames[i].commandPool);

    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = frames[i].commandPool;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAssert(dispatch.allocateCommandBuffers(&cmdAllocInfo,
                                             &frames[i].commandBuffer));
  }
  // immediate
  vkAssert(dispatch.createCommandPool(&cmdPoolInfo, nullptr,
                                      &immediateData.commandPool));

  VkCommandBufferAllocateInfo immCmdAllocInfo = {};
  immCmdAllocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  immCmdAllocInfo.commandPool = immediateData.commandPool;
  immCmdAllocInfo.commandBufferCount = 1;
  immCmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  vkAssert(dispatch.allocateCommandBuffers(&immCmdAllocInfo,
                                           &immediateData.commandBuffer));
}

void LeafEngine::initSynchronization() {

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start as signaled

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreInfo.flags                 = 0;

  for (size_t i = 0; i < framesInFlight; i++) {
    vkAssert(dispatch.createFence(&fenceInfo, nullptr, &frames[i].renderFence));

    vkAssert(dispatch.createSemaphore(&semaphoreInfo, nullptr,
                                      &frames[i].renderSemaphore));
    vkAssert(dispatch.createSemaphore(&semaphoreInfo, nullptr,
                                      &frames[i].swapchainSemaphore));

    vulkanDestroyer.addFence(frames[i].renderFence);
    vulkanDestroyer.addSemaphore(frames[i].renderSemaphore);
    vulkanDestroyer.addSemaphore(frames[i].swapchainSemaphore);
  }

  vkAssert(dispatch.createFence(&fenceInfo, nullptr, &immediateData.fence));
  vulkanDestroyer.addFence(immediateData.fence);
}

void LeafEngine::initImGUI() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  // style.ScaleAllSizes(2);
  // style.FontScaleDpi = 2.0;

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
  poolInfo.poolSizeCount = (uint32_t)std::size(pool_sizes);
  poolInfo.pPoolSizes    = pool_sizes;

  vkAssert(dispatch.createDescriptorPool(&poolInfo, nullptr,
                                         &imguiContext.descriptorPool));

  VkPipelineCreateInfoKHR pipelineInfo = {};

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance                    = instance;
  initInfo.PhysicalDevice              = physicalDevice;
  initInfo.Device                      = device;
  initInfo.QueueFamily                 = graphicsQueueFamily;
  initInfo.Queue                       = graphicsQueue;
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
      &swapchainImageFormat;
  initInfo.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
  initInfo.Allocator       = nullptr;
  initInfo.CheckVkResultFn = vkAssert;

  if (!ImGui_ImplSDL3_InitForVulkan(window)) {
    fmt::println("ImGui_ImplSDL3_InitForVulkan failed");
  }
  if (!ImGui_ImplVulkan_Init(&initInfo)) {
    fmt::println("failed to init IMGUI");
  }

  vulkanDestroyer.addDescriptorPool(imguiContext.descriptorPool);
}

void LeafEngine::initPipeline() {

  VkShaderModule vertexShader =
      leafUtil::loadShaderModule("triangleMesh.vert", dispatch);
  VkShaderModule fragmentShader =
      leafUtil::loadShaderModule("triangle.frag", dispatch);

  VkPushConstantRange pushConstantRanges[2];

  pushConstantRanges[0].offset     = 0;
  pushConstantRanges[0].size       = sizeof(VertPushData);
  pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  // TODO: Fix allignment nicely
  constexpr size_t fragOffset      = (sizeof(VertPushData) + 15) & ~15;
  pushConstantRanges[1].offset     = fragOffset;
  pushConstantRanges[1].size       = sizeof(FragPushData);
  pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.flags = 0;
  pipelineLayoutInfo.setLayoutCount         = 1;
  pipelineLayoutInfo.pSetLayouts            = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 2;
  pipelineLayoutInfo.pPushConstantRanges    = pushConstantRanges;

  vkAssert(dispatch.createPipelineLayout(&pipelineLayoutInfo, nullptr,
                                         &pipelineLayout));

  PipelineBuilder pipelineBuilder = PipelineBuilder(dispatch);
  pipelineBuilder.setLayout(pipelineLayout);
  pipelineBuilder.setShaders(vertexShader, fragmentShader);
  pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
  // pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_LINE);
  pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  pipelineBuilder.disableMultiSampling();
  pipelineBuilder.disableBlending();
  pipelineBuilder.disableDepthTest();
  pipelineBuilder.setColorAttachmentFormat(drawImage.imageFormat);

  pipeline = pipelineBuilder.build();

  dispatch.destroyShaderModule(vertexShader, nullptr);
  dispatch.destroyShaderModule(fragmentShader, nullptr);

  vulkanDestroyer.addPipeline(pipeline);
  vulkanDestroyer.addPipelineLayout(pipelineLayout);
}

void LeafEngine::drawImGUI(VkCommandBuffer cmd, VkImageView targetImage) {

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  // UI
  ImGui::Begin("TRIANGLE SLIDER");
  ImGui::Text("slide:");
  ImGui::SliderFloat("RED", &rectangleColor.r, 0.f, 1.f);
  ImGui::SliderFloat("GREEN", &rectangleColor.g, 0.f, 1.f);
  ImGui::SliderFloat("BLUE", &rectangleColor.b, 0.f, 1.f);
  ImGui::ColorEdit3("RECCOLOR", glm::value_ptr(rectangleColor));
  ImGui::ColorEdit3("BGCOLOR", glm::value_ptr(backgroundColor));

  ImGui::End();

  ImGui::Render();

  VkRenderingAttachmentInfo colorAttachment = {};
  colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView   = targetImage;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
  colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

  VkRenderingInfo renderInfo      = {};
  renderInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderInfo.renderArea           = VkRect2D{VkOffset2D{0, 0}, swapchainExtent};
  renderInfo.layerCount           = 1;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments    = &colorAttachment;
  renderInfo.pDepthAttachment     = nullptr;
  renderInfo.pStencilAttachment   = nullptr;

  dispatch.cmdBeginRendering(cmd, &renderInfo);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  dispatch.cmdEndRendering(cmd);
}

void LeafEngine::draw() {
  // wait for GPU to finish work
  vkAssert(dispatch.waitForFences(1, &getCurrentFrame().renderFence, true,
                                  1'000'000'000));
  vkAssert(dispatch.resetFences(1, &getCurrentFrame().renderFence));

  drawExtent.width  = swapchainExtent.width;
  drawExtent.height = swapchainExtent.height;

  uint32_t                  swapchainImageIndex;
  VkAcquireNextImageInfoKHR acquireInfo = {};
  acquireInfo.sType      = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
  acquireInfo.swapchain  = swapchain;
  acquireInfo.timeout    = 1'000'000'000; // 1 second
  acquireInfo.semaphore  = getCurrentFrame().swapchainSemaphore;
  acquireInfo.fence      = nullptr;
  acquireInfo.deviceMask = 0x1;

  vkAssert(dispatch.acquireNextImage2KHR(&acquireInfo, &swapchainImageIndex));

  VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
  vkAssert(dispatch.resetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkAssert(dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));

  leafUtil::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_GENERAL);

  drawBackground(cmd);

  leafUtil::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  drawGeometry(cmd, swapchainImageIndex);

  leafUtil::transitionImage(cmd, drawImage.image,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  leafUtil::transitionImage(cmd, swapchainImages[swapchainImageIndex],
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  leafUtil::copyImageToImage(cmd, drawImage.image,
                             swapchainImages[swapchainImageIndex], drawExtent,
                             swapchainExtent);

  leafUtil::transitionImage(cmd, swapchainImages[swapchainImageIndex],
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  drawImGUI(cmd, swapchainImageViews[swapchainImageIndex]);
  leafUtil::transitionImage(cmd, swapchainImages[swapchainImageIndex],
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  vkAssert(dispatch.endCommandBuffer(cmd));

  // command is ready for submission
  VkCommandBufferSubmitInfo submitInfo = {};
  submitInfo.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  submitInfo.commandBuffer = cmd;

  VkSemaphoreSubmitInfo waitInfo = {};
  waitInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  waitInfo.semaphore             = getCurrentFrame().swapchainSemaphore;
  waitInfo.value                 = 1;
  waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;

  VkSemaphoreSubmitInfo signalInfo = {};
  signalInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  signalInfo.semaphore             = getCurrentFrame().renderSemaphore;
  signalInfo.value                 = 1;
  signalInfo.stageMask             = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

  VkSubmitInfo2 submit            = {};
  submit.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit.waitSemaphoreInfoCount   = 1;
  submit.pWaitSemaphoreInfos      = &waitInfo;
  submit.commandBufferInfoCount   = 1;
  submit.pCommandBufferInfos      = &submitInfo;
  submit.signalSemaphoreInfoCount = 1;
  submit.pSignalSemaphoreInfos    = &signalInfo;

  vkAssert(dispatch.queueSubmit2(graphicsQueue, 1, &submit,
                                 getCurrentFrame().renderFence));

  // prepare present
  VkPresentInfoKHR presentInfo   = {};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pSwapchains        = &swapchain.swapchain;
  presentInfo.swapchainCount     = 1;
  presentInfo.pWaitSemaphores    = &getCurrentFrame().renderSemaphore;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pImageIndices      = &swapchainImageIndex;
  vkAssert(dispatch.queuePresentKHR(graphicsQueue, &presentInfo));

  frameNumber++;
}

void LeafEngine::drawBackground(VkCommandBuffer cmd) {

  VkClearColorValue bg = {backgroundColor.r, backgroundColor.g,
                          backgroundColor.b, backgroundColor.a};

  VkImageSubresourceRange clearRange =
      leafInit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

  dispatch.cmdClearColorImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
                              &bg, 1, &clearRange);
}

void LeafEngine::drawGeometry(VkCommandBuffer cmd,
                              uint32_t        swapchainImageIndex) {
  VkRenderingAttachmentInfo colorAttachment = {};
  colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView   = drawImage.imageView;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
  colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue  = {};

  VkRenderingInfo renderInfo      = {};
  renderInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderInfo.renderArea           = VkRect2D{VkOffset2D{0, 0}, drawExtent};
  renderInfo.layerCount           = 1;
  renderInfo.colorAttachmentCount = 1;
  renderInfo.pColorAttachments    = &colorAttachment;
  renderInfo.pDepthAttachment     = nullptr;
  renderInfo.pStencilAttachment   = nullptr;

  dispatch.cmdBeginRendering(cmd, &renderInfo);
  dispatch.cmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VertPushData mesh        = {};
  mesh.model               = glm::mat4{1.f};
  mesh.vertexBufferAddress = rectangle.vertexBufferAddress;

  CameraUBO* mapped = (CameraUBO*)cameraBuffers[frameNumber % framesInFlight]
                          .allocation->GetMappedData();
  mapped->view       = camera.getViewMatrix();
  mapped->projection = camera.getProjectionMatrix();

  dispatch.cmdBindDescriptorSets(
      cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
      &descriptorSets[frameNumber % framesInFlight], 0, nullptr);
  VkPushConstantsInfo vertPushInfo = {};
  vertPushInfo.sType               = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
  vertPushInfo.layout              = pipelineLayout;
  vertPushInfo.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
  vertPushInfo.offset              = 0;
  vertPushInfo.size                = sizeof(VertPushData);
  vertPushInfo.pValues             = &mesh;

  dispatch.cmdPushConstants(cmd, vertPushInfo.layout, vertPushInfo.stageFlags,
                            vertPushInfo.offset, vertPushInfo.size,
                            vertPushInfo.pValues);
  // dispatch.cmdPushConstants2(cmd, &vertPushInfo)
  // does not work, Missing extension (?)

  FragPushData fragColor = {rectangleColor};

  constexpr size_t    fragOffset   = (sizeof(VertPushData) + 15) & ~15;
  VkPushConstantsInfo fragPushInfo = {};
  fragPushInfo.sType               = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
  fragPushInfo.layout              = pipelineLayout;
  fragPushInfo.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragPushInfo.offset              = fragOffset;
  fragPushInfo.size                = sizeof(FragPushData);
  fragPushInfo.pValues             = &fragColor;

  dispatch.cmdPushConstants(cmd, fragPushInfo.layout, fragPushInfo.stageFlags,
                            fragPushInfo.offset, fragPushInfo.size,
                            fragPushInfo.pValues);
  // dispatch.cmdPushConstants2(cmd, &fragPushInfo);

  dispatch.cmdBindIndexBuffer(cmd, rectangle.indexBuffer.buffer, 0,
                              VK_INDEX_TYPE_UINT32);
  VkViewport viewport = {};
  viewport.x          = 0;
  viewport.y          = 0;
  viewport.width      = drawExtent.width;
  viewport.height     = drawExtent.height;
  viewport.minDepth   = 0.f;
  viewport.maxDepth   = 1.f;
  dispatch.cmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor      = {};
  scissor.offset.x      = 0;
  scissor.offset.y      = 0;
  scissor.extent.width  = drawExtent.width;
  scissor.extent.height = drawExtent.height;
  dispatch.cmdSetScissor(cmd, 0, 1, &scissor);

  dispatch.cmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
  dispatch.cmdEndRendering(cmd);
}

AllocatedBuffer LeafEngine::allocateBuffer(size_t             allocSize,
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

  vkAssert(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo,
                           &newBuffer.buffer, &newBuffer.allocation,
                           &newBuffer.allocationInfo));

  return newBuffer;
}

GPUMeshBuffers LeafEngine::uploadMesh(std::span<uint32_t> indices,
                                      std::span<Vertex>   vertices) {
  const size_t   vertexBufferSize = vertices.size() * sizeof(Vertex);
  const size_t   indexBufferSize  = indices.size() * sizeof(uint32_t);
  GPUMeshBuffers newSurface;

  newSurface.vertexbuffer = allocateBuffer(
      vertexBufferSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo deviceAddressInfo = {};
  deviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  deviceAddressInfo.buffer = newSurface.vertexbuffer.buffer;
  newSurface.vertexBufferAddress =
      dispatch.getBufferDeviceAddress(&deviceAddressInfo);

  newSurface.indexBuffer = allocateBuffer(indexBufferSize,
                                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          VMA_MEMORY_USAGE_GPU_ONLY);

  AllocatedBuffer staging = allocateBuffer(vertexBufferSize + indexBufferSize,
                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_MEMORY_USAGE_CPU_ONLY);

  void* data = staging.allocation->GetMappedData();
  memcpy(data, vertices.data(), vertexBufferSize);
  memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

  // immediate submit
  vkAssert(dispatch.resetFences(1, &immediateData.fence));
  vkAssert(dispatch.resetCommandBuffer(immediateData.commandBuffer, 0));

  VkCommandBuffer cmd = immediateData.commandBuffer;

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkAssert(dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));

  VkBufferCopy2 vertexCopy = {};
  vertexCopy.sType         = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
  vertexCopy.size          = vertexBufferSize;

  VkCopyBufferInfo2 vertexCopyInfo = {};
  vertexCopyInfo.sType             = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
  vertexCopyInfo.srcBuffer         = staging.buffer;
  vertexCopyInfo.dstBuffer         = newSurface.vertexbuffer.buffer;
  vertexCopyInfo.regionCount       = 1;
  vertexCopyInfo.pRegions          = &vertexCopy;

  dispatch.cmdCopyBuffer2(cmd, &vertexCopyInfo);

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

  dispatch.cmdCopyBuffer2(cmd, &indexCopyInfo);

  vkAssert(dispatch.endCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmdSubmitInfo = {};
  cmdSubmitInfo.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdSubmitInfo.commandBuffer = cmd;

  VkSubmitInfo2 submit          = {};
  submit.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos    = &cmdSubmitInfo;

  vkAssert(
      dispatch.queueSubmit2(graphicsQueue, 1, &submit, immediateData.fence));
  vkAssert(dispatch.waitForFences(1, &immediateData.fence, true, UINT64_MAX));

  vmaDestroyBuffer(allocator, staging.buffer, staging.allocation);

  return newSurface;
}

void LeafEngine::initMesh() {

  std::array<Vertex, 4> rectVertices;

  rectVertices[0].position = {0.5, -0.5, 0};
  rectVertices[1].position = {0.5, 0.5, 0};
  rectVertices[2].position = {-0.5, -0.5, 0};
  rectVertices[3].position = {-0.5, 0.5, 0};

  // rectVertices[0].color = {0, 0, 0, 1};
  // rectVertices[1].color = {0.5, 0.5, 0.5, 0};
  // rectVertices[2].color = {1, 0, 0, 1};
  // rectVertices[3].color = {0, 1, 0, 1};

  std::array<uint32_t, 6> rectIndices;

  rectIndices[0] = 0;
  rectIndices[1] = 1;
  rectIndices[2] = 2;

  rectIndices[3] = 2;
  rectIndices[4] = 1;
  rectIndices[5] = 3;

  rectangle = uploadMesh(rectIndices, rectVertices);
  fmt::println("indexBuffer: {}", (void*)rectangle.indexBuffer.buffer);
  fmt::println("vertexBuffer: {}", (void*)rectangle.vertexbuffer.buffer);

  vulkanDestroyer.addBuffer(rectangle.indexBuffer);
  vulkanDestroyer.addBuffer(rectangle.vertexbuffer);
}

void LeafEngine::processEvent(SDL_Event& e) { camera.processSDLEvent(e); }

void LeafEngine::update() { camera.update(); }

void LeafEngine::initDescriptorPool() {

  VkDescriptorPoolSize poolSizes[] = {{
      .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1 * framesInFlight,
  }};

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes    = poolSizes;
  poolInfo.maxSets       = framesInFlight;

  vkAssert(dispatch.createDescriptorPool(&poolInfo, nullptr, &descriptorPool));
}
void LeafEngine::initDescriptorLayout() {

  VkDescriptorSetLayoutBinding bindings[] = {
      {.binding            = 0,
       .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount    = 1,
       .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
       .pImmutableSamplers = nullptr}};

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings    = bindings;

  vkAssert(dispatch.createDescriptorSetLayout(&layoutInfo, nullptr,
                                              &descriptorSetLayout));
}

void LeafEngine::initCamera() {
  camera        = Camera{};
  camera.frames = framesInFlight;

  camera.aspectRatio = swapchainExtent.width / (float)swapchainExtent.height;
  fmt::println("Asp; {}", camera.aspectRatio);
  cameraBuffers.resize(framesInFlight);
  for (size_t i = 0; i < framesInFlight; i++) {
    cameraBuffers[i] =
        allocateBuffer(sizeof(CameraUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VMA_MEMORY_USAGE_CPU_TO_GPU);
  }
}

void LeafEngine::initDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(framesInFlight,
                                             descriptorSetLayout);
  VkDescriptorSetAllocateInfo        allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = descriptorPool;
  allocInfo.descriptorSetCount = framesInFlight;
  allocInfo.pSetLayouts        = layouts.data();

  descriptorSets.resize(framesInFlight);
  vkAssert(dispatch.allocateDescriptorSets(&allocInfo, descriptorSets.data()));

  for (size_t i = 0; i < framesInFlight; i++) {
    VkDescriptorBufferInfo cameraBufferInfo{};
    cameraBufferInfo.buffer = cameraBuffers[i].buffer;
    cameraBufferInfo.offset = 0;
    cameraBufferInfo.range  = sizeof(CameraUBO);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet          = descriptorSets[i];
    descriptorWrite.dstBinding      = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.pBufferInfo     = &cameraBufferInfo;

    dispatch.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
  }
}
