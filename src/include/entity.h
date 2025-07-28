
#include "leafStructs.h"

constexpr u32 ENTITY_TYPE_NONE   = 0;
constexpr u32 ENTITY_TYPE_PLANET = 1 << 0;
constexpr u32 ENTITY_TYPE_TEAPOT = 1 << 1;

struct Entity {
  MeshAsset mesh;
  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 scale;
  // glm::vec4  color;
  glm::mat4 model;
  u32       type;
};
