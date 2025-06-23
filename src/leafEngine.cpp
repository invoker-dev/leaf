#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <fmt/printf.h>
#include <leafEngine.h>

constexpr bool useValidationLayers = true;

LeafEngine::LeafEngine() {

  init.windowExtent = {.width = 480, .height = 480};

  createSDLWindow();
  initVulkan();
  getQueues();

  int w, h;
  SDL_GetWindowSize(init.window, &w, &h);
  fmt::println("Window size: {} x {}", w, h);


}
LeafEngine::~LeafEngine() {
  vkb::destroy_device(init.device);
  vkb::destroy_surface(init.instance, init.surface);
  vkb::destroy_instance(init.instance);
  SDL_DestroyWindow(init.window);
  SDL_Quit();
}

void LeafEngine::createSDLWindow() {

  init.window = SDL_CreateWindow("LeafEngine", init.windowExtent.width,
                                 init.windowExtent.height, SDL_WINDOW_VULKAN | SDL_WINDOW_BORDERLESS);
  if (!init.window) {
    fmt::println("failed to init SDL window: {}", SDL_GetError());
    std::exit(-1);
  }

  if (!SDL_ShowWindow(init.window)) {
    fmt::println("failed to show SDL window: {}", SDL_GetError());
    std::exit(-1);
  }

  fmt::println("Window pointer: {}", (void*)init.window);


}

void LeafEngine::initVulkan() {

  vkb::InstanceBuilder builder;

  vkb::Result<vkb::Instance> returnedInstance =
      builder.set_app_name("leaf")
          .require_api_version(1, 3, 0)
          .use_default_debug_messenger()
          .request_validation_layers(useValidationLayers)
          .build();

  if (!returnedInstance) {
    fmt::println("Failed to build vkbInstance: {}",
                returnedInstance.error().message());
    std::exit(-1);
  }

  init.instance = returnedInstance.value();

  bool result = SDL_Vulkan_CreateSurface(init.window, init.instance.instance,
                                         nullptr, &init.surface);
  if (!result) {
    fmt::println("failed to create SDL surface: {}", SDL_GetError());
  }

  // init.InstanceDispatchTable = init.instance.make_table();

  VkPhysicalDeviceVulkan13Features features13{};
  features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features13.dynamicRendering = true; // allows us to skip render pass
  features13.synchronization2 = true;

  VkPhysicalDeviceVulkan12Features features12{};
  features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing  = true;

  vkb::PhysicalDeviceSelector selector{init.instance};
  auto returnedPhysicalDevice = selector.set_minimum_version(1, 3)
                                    .set_required_features_13(features13)
                                    .set_required_features_12(features12)
                                    .set_surface(init.surface)
                                    .select();

  if (!returnedPhysicalDevice) {
    fmt::println("Failed to find physical device: {}",
                returnedPhysicalDevice.error().message());
    std::exit(-1);
  }

  init.physicalDevice = returnedPhysicalDevice.value();

  vkb::DeviceBuilder deviceBuilder{init.physicalDevice};
  auto               returnedDevice = deviceBuilder.build();

  if (!returnedDevice) {
    fmt::println("Failed to build device: {}", returnedDevice.error().message());
    std::exit(-1);
  }
  init.device = returnedDevice.value();
}

void LeafEngine::getQueues() {
  auto graphicsQueue = init.device.get_queue(vkb::QueueType::graphics);
  if (!graphicsQueue.has_value()) {
    fmt::println("failed to get graphics queue: {}",
                graphicsQueue.error().message());
    std::exit(-1);
  }
  auto presentQueue = init.device.get_queue(vkb::QueueType::present);
  if (!presentQueue.has_value()) {
    fmt::println("failed to get present queue: {}",
                presentQueue.error().message());
    std::exit(-1);
  }
}
