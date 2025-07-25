#include "leafStructs.h"
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <random>
#include <types.h>
#include <vector>
#include <vulkan/vulkan_core.h>

struct InstanceData {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> rotations;
  std::vector<glm::vec3> scales;
  std::vector<glm::vec4> colors;
  std::vector<glm::mat4> modelMatrices;
  u32                    count = 0;
};

class CubeSystem {
public:
  CubeSystem();
  void addCubes(u32 count);

  InstanceData data;
  MeshAsset mesh;
  std::mt19937 random;

  u32 maxInstances;
  u32 initialCubeAmount;
  u32 initialSeed = 0;
};
