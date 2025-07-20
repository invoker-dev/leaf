#include "VkBootstrapDispatch.h"
#include <cstdint>
#include <cube.h>
#include <glm/ext/scalar_constants.hpp>
#include <random>
#include <xmmintrin.h>

namespace CubeSystem {

// Random Number Generator from
// pcg-random.org
inline float pcg64(uint32_t state) {
  uint64_t word =
      ((state >> ((state >> 59u) + 5u)) ^ state) * 12605985483714917081ull;
  return ((word >> 43u) ^ word) / (float)(1ULL << 32); // [0,1)
}

void System::init() {
  initialCubeAmount = 2 << 8;
  data.positions.reserve(initialCubeAmount);
  data.rotations.reserve(initialCubeAmount);
  data.scales.reserve(initialCubeAmount);
  data.colors.reserve(initialCubeAmount);
  data.modelMatrices.reserve(initialCubeAmount);
  addCubes(initialCubeAmount);
};

void System::destroy() {};
void System::addCubes(uint32_t count) {
  size_t   startIndex = data.positions.size();
  uint64_t seedCounter;

  if (initialSeed = 0) {
    seedCounter = std::random_device{}();
  } else {
    seedCounter = initialSeed;
  }

  data.positions.resize(startIndex + count);
  data.rotations.resize(startIndex + count);
  data.scales.resize(startIndex + count);
  data.colors.resize(startIndex + count);

  for (size_t i = 0; i < count; i++) {
    data.positions[startIndex + i].x = pcg64(seedCounter++) * 20.f - 10.f;
    data.positions[startIndex + i].y = pcg64(seedCounter++) * 20.f - 10.f;
    data.positions[startIndex + i].z = pcg64(seedCounter++) * 20.f - 10.f;
  }

  float pi = glm::pi<float>();
  for (size_t i = 0; i < count; i++) {
    data.rotations[startIndex + i].x = pcg64(seedCounter++) * 2 * pi;
    data.rotations[startIndex + i].y = pcg64(seedCounter++) * 2 * pi;
    data.rotations[startIndex + i].z = pcg64(seedCounter++) * 2 * pi;
  }

  for (size_t i = 0; i < count; i++) {
    data.scales[startIndex + i].x = pcg64(seedCounter++) * 2.f + 0.5f;
    data.scales[startIndex + i].y = pcg64(seedCounter++) * 2.f + 0.5f;
    data.scales[startIndex + i].z = pcg64(seedCounter++) * 2.f + 0.5f;
  }

  for (size_t i = 0; i < count; i++) {
    data.colors[startIndex + i].r = pcg64(seedCounter++);
    data.colors[startIndex + i].g = pcg64(seedCounter++);
    data.colors[startIndex + i].b = pcg64(seedCounter++);
    data.colors[startIndex + i].a = 1.0f;
  }

  data.count += count;
};

void System::render() {
  // ?
};
} // namespace CubeSystem
