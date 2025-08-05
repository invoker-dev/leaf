#pragma once
#include "leafStructs.h"
#include <constants.h>

struct EntityData {
  MeshAsset mesh;
  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 scale;
  glm::vec4 color;
  f32       tint;
  glm::mat4 model;
  AllocatedImage texture;
  
};
