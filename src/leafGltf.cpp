#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp> // for glm support (optional)
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/util.hpp>
#include <filesystem>
#include <fmt/base.h>
#include <iostream>
#include <leafGltf.h>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace leafGltf {

MeshAsset loadGltfMesh(std::filesystem::path filePath) {
  //> openmesh
  std::cout << "Loading GLTF: " << filePath << std::endl;
  std::cout << "Loading GLTF: " << std::filesystem::absolute(filePath)
            << std::endl;

  fastgltf::GltfFileStream data(filePath);

  constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

  fastgltf::Asset  gltf;
  fastgltf::Parser parser{};

  // auto load = parser.loadGltfBinary(data, filePath.parent_path(),
  // gltfOptions);
  auto load = parser.loadGltfJson(data, filePath.parent_path(), gltfOptions);
  if (load) {
    gltf = std::move(load.get());
  } else {
    fmt::print("Failed to load glTF: {} \n",
               fastgltf::to_underlying(load.error()));
    return {};
  }
  //< openmesh
  //> loadmesh
  std::vector<MeshAsset> meshes;

  // use the same vectors for all meshes so that the memory doesnt reallocate
  // as often
  std::vector<uint32_t> indices;
  std::vector<Vertex>   vertices;
  for (fastgltf::Mesh& mesh : gltf.meshes) {
    MeshAsset newmesh;

    newmesh.name = mesh.name;

    // clear the mesh arrays each mesh, we dont want to merge them by error
    indices.clear();
    vertices.clear();

    for (auto&& p : mesh.primitives) {
      GeoSurface newSurface;
      newSurface.startIndex = (uint32_t)indices.size();
      newSurface.count =
          (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

      size_t initial_vtx = vertices.size();

      // load indexes
      {
        fastgltf::Accessor& indexaccessor =
            gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(
            gltf, indexaccessor,
            [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
      }

      // load vertex positions
      {

        fastgltf::Accessor& posAccessor =
            gltf.accessors.at(p.findAttribute("POSITION")->accessorIndex);

        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, posAccessor, [&](glm::vec3 v, size_t index) {
              Vertex newvtx;
              newvtx.position               = v;
              newvtx.normal                 = {1, 0, 0};
              newvtx.color                  = glm::vec4{1.f};
              newvtx.uv_x                   = 0;
              newvtx.uv_y                   = 0;
              vertices[initial_vtx + index] = newvtx;
            });
      }

      // load vertex normals
      auto normals = p.findAttribute("NORMAL");
      if (normals != p.attributes.end()) {

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, gltf.accessors.at(normals->accessorIndex),
            [&](glm::vec3 v, size_t index) {
              vertices[initial_vtx + index].normal = v;
            });
      }

      // load UVs
      auto uv = p.findAttribute("TEXCOORD_0");
      if (uv != p.attributes.end()) {

        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            gltf, gltf.accessors.at(uv->accessorIndex),
            [&](glm::vec2 v, size_t index) {
              vertices[initial_vtx + index].uv_x = v.x;
              vertices[initial_vtx + index].uv_y = v.y;
            });
      }

      // load vertex colors
      auto colors = p.findAttribute("COLOR_0");
      if (colors != p.attributes.end()) {

        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            gltf, gltf.accessors.at(colors->accessorIndex),
            [&](glm::vec4 v, size_t index) {
              vertices[initial_vtx + index].color = v;
            });
      }
      newmesh.surfaces.push_back(newSurface);
    }

    // display the vertex normals
    constexpr bool OverrideColors = true;
    if (OverrideColors) {
      for (Vertex& vtx : vertices) {
        vtx.color = glm::vec4(vtx.position, 1.f);
      }
    }
    // newmesh.meshBuffers = engine.uploadMesh(indices, vertices);

    newmesh.indices  = indices;
    newmesh.vertices = vertices;
    meshes.emplace_back(newmesh);
  }

  // TODO: Figure this out
  return meshes[0];

  //< loadmesh
}
} // namespace leafGltf
