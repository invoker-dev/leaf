#include <SDL3/SDL_events.h>
#include <cstdint>
#include <cube.h>
#include <fmt/base.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <random>
#include <xmmintrin.h>

CubeSystem::CubeSystem() {

  random = std::mt19937(std::random_device{}());

  // initialCubeAmount = 2 << 9;
  initialCubeAmount = 1;
  data.positions.reserve(initialCubeAmount);
  data.rotations.reserve(initialCubeAmount);
  data.scales.reserve(initialCubeAmount);
  data.colors.reserve(initialCubeAmount);
  data.modelMatrices.reserve(initialCubeAmount);

  addCubes(initialCubeAmount);

  mesh.vertices = std::vector<Vertex>(8);

  // Positions
  mesh.vertices[0].position = {-0.5f, -0.5f, 0.5f};
  mesh.vertices[1].position = {0.5f, -0.5f, 0.5f};
  mesh.vertices[2].position = {0.5f, 0.5f, 0.5f};
  mesh.vertices[3].position = {-0.5f, 0.5f, 0.5f};
  mesh.vertices[4].position = {-0.5f, -0.5f, -0.5f};
  mesh.vertices[5].position = {0.5f, -0.5f, -0.5f};
  mesh.vertices[6].position = {0.5f, 0.5f, -0.5f};
  mesh.vertices[7].position = {-0.5f, 0.5f, -0.5f};

  // UV coordinates for front face (0-3)
  mesh.vertices[0].uv_x = 0.0f;
  mesh.vertices[0].uv_y = 0.0f;
  mesh.vertices[1].uv_x = 1.0f;
  mesh.vertices[1].uv_y = 0.0f;
  mesh.vertices[2].uv_x = 1.0f;
  mesh.vertices[2].uv_y = 1.0f;
  mesh.vertices[3].uv_x = 0.0f;
  mesh.vertices[3].uv_y = 1.0f;

  // UV coordinates for back face (4-7)
  mesh.vertices[4].uv_x = 1.0f;
  mesh.vertices[4].uv_y = 0.0f;
  mesh.vertices[5].uv_x = 0.0f;
  mesh.vertices[5].uv_y = 0.0f;
  mesh.vertices[6].uv_x = 0.0f;
  mesh.vertices[6].uv_y = 1.0f;
  mesh.vertices[7].uv_x = 1.0f;
  mesh.vertices[7].uv_y = 1.0f;

  // Normals (front face = +Z, back face = -Z)
  for (int i = 0; i < 4; i++) {
    mesh.vertices[i].normal = {0.0f, 0.0f, 1.0f}; // Front face
  }
  for (int i = 4; i < 8; i++) {
    mesh.vertices[i].normal = {0.0f, 0.0f, -1.0f}; // Back face
  }

  mesh.indices = {// Front face
                  0, 1, 2, 2, 3, 0,
                  // Back face
                  4, 6, 5, 6, 4, 7,
                  // Left face
                  4, 0, 3, 3, 7, 4,
                  // Right face
                  1, 5, 6, 6, 2, 1,
                  // Top face
                  3, 2, 6, 6, 7, 3,
                  // Bottom face
                  4, 5, 1, 1, 0, 4};
};

void CubeSystem::addCubes(uint32_t count) {

  size_t startIndex = data.positions.size();

  std::uniform_real_distribution<> dist(0, 1);

  data.positions.resize(startIndex + count);
  data.rotations.resize(startIndex + count);
  data.scales.resize(startIndex + count);
  data.colors.resize(startIndex + count);
  data.modelMatrices.resize(startIndex + count);

  for (size_t i = startIndex; i < startIndex + count; i++) {
    data.positions[i].x =
        dist(random) * initialCubeAmount / ((f32)initialCubeAmount / 2) - 10.f;
    data.positions[i].y =
        dist(random) * initialCubeAmount / ((f32)initialCubeAmount / 2) - 10.f;
    data.positions[i].z =
        dist(random) * initialCubeAmount / ((f32)initialCubeAmount / 2) - 10.f;
  }

  f32 pi = glm::pi<f32>();
  for (size_t i = startIndex; i < startIndex + count; i++) {
    // data.rotations[i].x = dist(random) * 2 * pi;
    // data.rotations[i].x = dist(random) * 2 * pi;
    // data.rotations[i].y = dist(random) * 2 * pi;
    data.rotations[i].z = 0;
    data.rotations[i].y = 0;
    data.rotations[i].z = 0; 
  }

  for (size_t i = startIndex; i < startIndex + count; i++) {
    // data.scales[i].x = dist(random) * 4.f + 0.5f;
    // data.scales[i].y = dist(random) * 4.f + 0.5f;
    // data.scales[i].z = dist(random) * 4.f + 0.5f;
    data.scales[i].x = 4.f + 0.5f;
    data.scales[i].y = 4.f + 0.5f;
    data.scales[i].z = 4.f + 0.5f;
  }

  for (size_t i = startIndex; i < startIndex + count; i++) {
    size_t index;
    data.colors[i].r = dist(random);
    data.colors[i].g = dist(random);
    data.colors[i].b = dist(random);
    data.colors[i].a = 1.0f;
  }

  for (size_t i = startIndex; i < startIndex + count; i++) {
    glm::mat4 model = glm::mat4(1.f);

    model = glm::scale(model, data.scales[i]);
    model = glm::rotate(model, data.rotations[i].x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, data.rotations[i].y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, data.rotations[i].z, glm::vec3(0, 0, 1));
    model = glm::translate(model, data.positions[i]);

    data.modelMatrices[i] = model;
  }

  data.count += count;

  // debugging
  // for (size_t i = 0; i < count; i++) {
  //   fmt::println("cube {}: [{},{},{}]", i, data.positions[i].x,
  //                data.positions[i].y, data.positions[i].z);
  // }
};
