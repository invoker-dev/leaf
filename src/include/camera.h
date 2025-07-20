#include <SDL3/SDL_events.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/vec3.hpp>>
class Camera {
public:
  Camera() {
    position    = glm::vec3(0);
    velocity    = glm::vec3(0);
    pitch       = 0;
    yaw         = 0;
    sensitivity = .001f;
    frames      = 0;
    aspectRatio = 0;
  };
  ~Camera() {};
  glm::mat4 getViewMatrix();
  glm::mat4 getRotationMatrix();
  glm::mat4 getProjectionMatrix();
  void      processSDLEvent(SDL_Event& e);
  void      update();

  glm::vec3 position;
  glm::vec3 velocity;
  float     pitch;
  float     yaw;
  float     sensitivity;
  float     speed;

  float    aspectRatio;
  uint32_t frames;
};
