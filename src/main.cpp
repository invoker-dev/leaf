#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <fmt/base.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <fmt/core.h>
#include <leafEngine.h>
#include <types.h>

LeafEngine::Engine* engine;

f64 dt          = 1 / 120.0;
f64 time        = 0;
f64 currentTime = SDL_GetPerformanceCounter();
// init
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {

  engine = new LeafEngine::Engine{};

  // dt
  return SDL_APP_CONTINUE;
}
// input
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {

  engine->processEvent(*event);

  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
  }
  return SDL_APP_CONTINUE;
}

// update
SDL_AppResult SDL_AppIterate(void* appstate) {

  double newTime =
      SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
  double frameTime = newTime - currentTime;
  currentTime      = newTime;

  while (frameTime > 0) {
    float deltaTime = std::min(frameTime, dt);
    engine->update(deltaTime);
    frameTime -= deltaTime;
    time += deltaTime;
  }

  engine->draw();

  return SDL_APP_CONTINUE;
}

// cleanup
void SDL_AppQuit(void* appstate, SDL_AppResult result) { delete engine; }
