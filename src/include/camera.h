#include <SDL3/SDL_events.h>
#include <body.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/vec3.hpp>
#include <types.h>

class Camera {
public:
  Camera();
  ~Camera() {};
  glm::mat4 getViewMatrix();
  glm::mat4 getRotationMatrix();
  glm::mat4 getProjectionMatrix();
  void      setTarget(Body& body);
  void      noTarget() { target = nullptr; }
  void      handleMouse(SDL_Event& e);
  void      handleInput();
  void      update(f64 dt);

  Body*     target;
  f32 orbitDistance;
  glm::vec3 position;
  glm::vec3 velocity;
  f32       pitch;
  f32       yaw;
  f32       sensitivity;
  f32       speed;

  f32  near;
  f32  far;
  f32  aspectRatio;
  u32  frames;
  bool active;
};
