#include "leafStructs.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_scancode.h>
#include <camera.h>
#include <fmt/base.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
void Camera::update() {

  glm::mat4 cameraRotation = getRotationMatrix();
  position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.5f, 0.f));
}

void Camera::processSDLEvent(SDL_Event& e) {

  SDL_Scancode scancode = e.key.scancode;

  if (e.type == SDL_EVENT_KEY_DOWN) {
    if (scancode == SDL_SCANCODE_W) {
      velocity.z = -1;
    }
    if (scancode == SDL_SCANCODE_S) {
      velocity.z = 1;
    }
    if (scancode == SDL_SCANCODE_A) {
      velocity.x = -1;
    }
    if (scancode == SDL_SCANCODE_D) {
      velocity.x = 1;
    }
    if(scancode == SDL_SCANCODE_SPACE){
      velocity.y = 0.3;
    }
    if(scancode == SDL_SCANCODE_LCTRL){
      velocity.y = -0.3;
    }

  }

  if (e.type == SDL_EVENT_KEY_UP) {
    if (scancode == SDL_SCANCODE_W) {
      velocity.z = 0;
    }
    if (scancode == SDL_SCANCODE_S) {
      velocity.z = 0;
    }
    if (scancode == SDL_SCANCODE_A) {
      velocity.x = 0;
    }
    if (scancode == SDL_SCANCODE_D) {
      velocity.x = 0;
    }
    if(scancode == SDL_SCANCODE_SPACE){
      velocity.y = 0;
    }
    if(scancode == SDL_SCANCODE_LCTRL){
      velocity.y = 0;
    }
  }


  if (e.type == SDL_EVENT_MOUSE_MOTION) {
    pitch += (float)e.motion.yrel * sensitivity;
    yaw -= (float)e.motion.xrel * sensitivity;
  }

  fmt::println("KEY PRESSED: {}", e.type);
}

glm::mat4 Camera::getViewMatrix() {
  glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
  glm::mat4 cameraRotation    = getRotationMatrix();
  return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::getRotationMatrix() {
  glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{1.f, 0.f, 0.f});
  glm::quat yawRotation   = glm::angleAxis(yaw, glm::vec3{0.f, 1.f, 0.f});
  return glm::mat4_cast(pitchRotation) * glm::mat4_cast(yawRotation);
}
glm::mat4 Camera::getProjectionMatrix() {

  return glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.f);
}
