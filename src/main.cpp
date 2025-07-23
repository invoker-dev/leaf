#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>
#include <fmt/base.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <fmt/core.h>
#include <leafEngine.h>

LeafEngine::Engine* engine;
// init
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {

  engine = new LeafEngine::Engine{};
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

  engine->update();
  engine->draw();

  return SDL_APP_CONTINUE;
}

// cleanup
void SDL_AppQuit(void* appstate, SDL_AppResult result) { delete engine; }
