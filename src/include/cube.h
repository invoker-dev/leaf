#include "leafStructs.h"
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace CubeSystem {

struct MeshGeometry {
  std::vector<Vertex>   vertices;
  std::vector<uint32_t> indices;
  GPUMeshBuffers        meshBuffers;

  VkBuffer& getIndexBuffer() { return meshBuffers.indexBuffer.buffer; }
  VkBuffer& getVertexBuffer() { return meshBuffers.vertexBuffer.buffer; }
};

struct InstanceData {
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> rotations;
  std::vector<glm::vec3> scales;
  std::vector<glm::vec4> colors;
  std::vector<glm::mat4> modelMatrices;
  uint32_t               count = 0;
};

struct System {
  InstanceData data;
  MeshGeometry mesh;
  uint32_t     maxInstances;
  uint32_t     initialCubeAmount;
  uint32_t     initialSeed = 0;

  void init();
  void destroy();
  void addCubes(uint32_t count);
  void render();
};

inline System& get() {
  static System cubeSystem;
  return cubeSystem;
}
} // namespace CubeSystem
