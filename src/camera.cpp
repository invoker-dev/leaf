#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_scancode.h>
#include <camera.h>
#include <cmath>
#include <cstdio>
#include <fmt/base.h>
#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <leafStructs.h>

Camera::Camera() {
  position      = glm::vec3(0, 0, 3000);
  velocity      = glm::vec3(0);
  pitch         = 0;
  yaw           = 0;
  sensitivity   = .001f;
  frames        = 0;
  aspectRatio   = 0;
  active        = true;
  speed         = 100;
  near          = 0.1f;
  far           = 100'000'00.f;
}

void Camera::update(f64 dt) {

  if (target) {
    glm::vec3 center = target->entityData.position;

    orbitDistance = 100 + target->baseScale * target->entityData.scale.length();

    f32 x = orbitDistance * cosf(pitch) * sinf(yaw);
    f32 y = orbitDistance * sinf(pitch);
    f32 z = orbitDistance * cosf(pitch) * cosf(yaw);

    position = center + glm::vec3(x, y, z);
  } else {

    glm::mat4 cameraRotation = getRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * (f32)dt, 0.f));
  }
}

void Camera::setTarget(Body& body) { target = &body; }

void Camera::handleMouse(SDL_Event& e) {

  if (active && e.type == SDL_EVENT_MOUSE_MOTION) {
    pitch = glm::clamp(pitch - (f32)e.motion.yrel * sensitivity,
                       glm::radians(-90.0f), glm::radians(90.0f));
    yaw -= (f32)e.motion.xrel * sensitivity;

    if (yaw > glm::two_pi<f32>())
      yaw -= glm::two_pi<f32>();
    if (yaw < 0.0f)
      yaw += glm::two_pi<f32>();
  }
}
void Camera::handleInput() {

  const bool* keys  = SDL_GetKeyboardState(NULL);
  glm::vec3   input = glm::vec3(0.f);
  if (keys[SDL_SCANCODE_W])
    input.z -= 1.f;
  if (keys[SDL_SCANCODE_S])
    input.z += 1.f;
  if (keys[SDL_SCANCODE_A])
    input.x -= 1.f;
  if (keys[SDL_SCANCODE_D])
    input.x += 1.f;
  if (keys[SDL_SCANCODE_SPACE])
    input.y += 1.f;
  if (keys[SDL_SCANCODE_LCTRL])
    input.y -= 1.f;
  if (glm::length(input) > 0.f)
    input = glm::normalize(input);

  velocity = input * speed;
}

glm::mat4 Camera::getViewMatrix() {
  if (target) {
    return glm::lookAt(position, target->entityData.position,
                       glm::vec3(0, 1, 0));
  } else {
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
    glm::mat4 cameraRotation    = getRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
  }
}

glm::mat4 Camera::getRotationMatrix() {
  glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{1.f, 0.f, 0.f});
  glm::quat yawRotation   = glm::angleAxis(yaw, glm::vec3{0.f, 1.f, 0.f});
  return glm::mat4_cast(yawRotation) * glm::mat4_cast(pitchRotation);
}
glm::mat4 Camera::getProjectionMatrix() {

  glm::mat4 p = glm::perspective(glm::radians(45.0f), aspectRatio, near, far);
  p[1][1]     = -p[1][1];
  return p;
}
