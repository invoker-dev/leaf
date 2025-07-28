#include "fastgltf/math.hpp"
#include "types.h"
#include <cstdio>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/util.hpp>
#include <fmt/base.h>
#include <leafStructs.h>
#include <unordered_map>
#include <utility>

namespace leafGltf {

MeshAsset loadGltfMesh(std::filesystem::path filePath);
} // namespace leafGltf
