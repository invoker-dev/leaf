#include "leafStructs.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fmt/base.h>
#include <fmt/printf.h>
#include <leafEngine.h>
#include <leafInit.h>
#include <leafUtil.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#define VMA_IMPLEMENTATION
#include <slang.h>
#include <vector>
#include <vk_mem_alloc.h>

LeafEngine::LeafEngine() {

  createSDLWindow();
  initVulkan();
  getQueues();
  initSwapchain();
  initCommands();
  initSynchronization();
}
LeafEngine::~LeafEngine() {

  vkAssert(dispatch.deviceWaitIdle());

  // destroys primitives (images, buffers, fences etc)
  vulkanDestroyer.flush(device, allocator);

  vmaDestroyAllocator(allocator);
  vkb::destroy_swapchain(swapchain);
  vkb::destroy_device(device);
  vkb::destroy_surface(instance, surface);
  vkb::destroy_instance(instance);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void LeafEngine::createSDLWindow() {

  SDL_WindowFlags sdlFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

  window = SDL_CreateWindow("LeafEngine", 600, 600, sdlFlags);
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
  auto graphicsQueue = device.get_queue(vkb::QueueType::graphics);
  if (!graphicsQueue.has_value()) {
    fmt::println("failed to get graphics queue: {}",
                 graphicsQueue.error().message());
    std::exit(-1);
  }
  graphicsQueue = graphicsQueue.value();
  graphicsQueueFamily =
      device.get_queue_index(vkb::QueueType::graphics).value();
}

void LeafEngine::initSwapchain() {

  int width, height;
  SDL_GetWindowSizeInPixels(window, &width, &height);
  windowExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  fmt::println("Window size:   [{} {}]", width, height);

  vkb::SwapchainBuilder swapchain_builder{physicalDevice, device, surface};
  auto                  returnedSwapchain =
      swapchain_builder.set_desired_format(surfaceFormat)
          .set_desired_format(VkSurfaceFormatKHR{
              .format     = swapchainImageFormat,
              .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .set_desired_extent(width, height)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build();

  if (!returnedSwapchain) {
    fmt::println("failed to build swapchain: {}",
                 returnedSwapchain.error().message());
    std::exit(-1);
  }
  vkb::destroy_swapchain(swapchain);

  swapchain                      = returnedSwapchain.value();
  swapchainImageViews = swapchain.get_image_views().value();
  swapchainImages     = swapchain.get_images().value();
  swapchainExtent     = swapchain.extent;

  VkExtent3D drawImageExtent = {};
  drawImageExtent.width      = windowExtent.width;
  drawImageExtent.height     = windowExtent.height;
  drawImageExtent.depth      = 1;

  drawImage             = {};
  drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
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
                 &drawImage.image, &drawImage.allocation,
                 nullptr);

  VkImageViewCreateInfo renderImageViewCreateInfo =
      leafInit::imageViewCreateInfo(drawImage.imageFormat,
                                    drawImage.image,
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
                                             &frames[i].mainCommandBuffer));
  }
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
}

void LeafEngine::draw() {
  // wait for GPU to finish work

  vkAssert(dispatch.waitForFences(1, &getCurrentFrame().renderFence, true,
                                  1'000'000'000));
  vkAssert(dispatch.resetFences(1, &getCurrentFrame().renderFence));

  uint32_t                  swapchainImageIndex;
  VkAcquireNextImageInfoKHR acquireInfo = {};
  acquireInfo.sType      = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
  acquireInfo.swapchain  = swapchain;
  acquireInfo.timeout    = 1'000'000'000; // 1 second
  acquireInfo.semaphore  = getCurrentFrame().swapchainSemaphore;
  acquireInfo.fence      = nullptr;
  acquireInfo.deviceMask = 0x1;

  vkAssert(dispatch.acquireNextImage2KHR(&acquireInfo, &swapchainImageIndex));

  // fmt::println("swapchainIndex: {} {}", frameNumber,
  //              swapchainImageIndex);
  // fmt::println("get curr frame: {}", (void*)&getCurrentFrame());
  // render commands
  VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;
  vkAssert(dispatch.resetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkAssert(dispatch.beginCommandBuffer(cmd, &cmdBeginInfo));

  // transition image to writable
  leafUtil::transitionImage(cmd, drawImage.image,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  drawBackground(cmd);

  // transition image to drawable
  leafUtil::transitionImage(cmd, drawImage.image,
                            VK_IMAGE_LAYOUT_GENERAL,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  leafUtil::transitionImage(
      cmd, swapchainImages[swapchainImageIndex],
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  leafUtil::copyImageToImage(cmd, drawImage.image,
                             swapchainImages[swapchainImageIndex],
                             drawExtent, swapchainExtent);

  leafUtil::transitionImage(
      cmd, swapchainImages[swapchainImageIndex],
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

  VkClearColorValue clearValue;

  float flash = std::abs(std::sin(frameNumber / 120.f));
  clearValue  = {{0.2, 0.1, flash, 1.0f}};

  VkImageSubresourceRange clearRange =
      leafInit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

  dispatch.cmdClearColorImage(cmd, drawImage.image,
                              VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1,
                              &clearRange);
}
