#include "fastgltf/math.hpp"
#include "leafEngine.h"
#include "types.h"
#include <cstdio>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/util.hpp>
#include <fmt/base.h>
#include <leafStructs.h>
#include <utility>

namespace leafGltf {


std::optional<std::vector<std::shared_ptr<MeshAsset>>>
loadGltfMeshes(std::filesystem::path filePath);
} // namespace leafGltf
