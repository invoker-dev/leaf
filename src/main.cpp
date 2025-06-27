#include <SDL3/SDL_events.h>
#include <fmt/base.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <fmt/core.h>
#include <leafEngine.h>

LeafEngine* engine;

// init
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {

  engine = new LeafEngine;
  return SDL_APP_CONTINUE;
}
// input
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {

  SDL_WaitEvent(event);

  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
  }
  return SDL_APP_CONTINUE;
}

// update
SDL_AppResult SDL_AppIterate(void* appstate) {

  engine->draw();

  fmt::println("draw");

  return SDL_APP_CONTINUE;
}

// cleanup
void SDL_AppQuit(void* appstate, SDL_AppResult result) { delete (engine); }
