// Builds renderable meshes (positions/normals/uvs/indices) from a parsed
// Drawable, ready to be uploaded into GPU buffers by the GUI viewport.
#pragma once

#include <cstdint>
#include <vector>

#include "evw/gamefiles/drawable.h"
#include "evw/math/math.h"

namespace evw::app
{
    struct RenderMesh
    {
        std::vector<math::Vector3> positions;
        std::vector<math::Vector3> normals;
        std::vector<math::Vector2> texCoords;
        std::vector<uint16_t> indices;
        uint16_t shaderId = 0;
        int diffuseTexture = -1;   // index into RenderModel::textures, or -1
        math::Vector3 bbMin;
        math::Vector3 bbMax;

        size_t triangleCount() const { return indices.size() / 3; }
    };

    // A decoded RGBA texture ready for GPU upload.
    struct RenderTexture
    {
        uint32_t nameHash = 0;
        int width = 0;
        int height = 0;
        std::vector<uint8_t> rgba;   // width*height*4
    };

    // A complete renderable model: meshes plus their referenced textures.
    struct RenderModel
    {
        std::vector<RenderMesh> meshes;
        std::vector<RenderTexture> textures;
    };

    // Extracts all geometry meshes from a drawable (all LOD models, all geometries).
    std::vector<RenderMesh> buildMeshes(const gamefiles::DrawableBase& drawable);

    // Extracts meshes from a single model.
    std::vector<RenderMesh> buildMeshes(const gamefiles::DrawableModel& model);

    // Builds meshes + resolves/decodes each mesh's diffuse texture from the
    // drawable's embedded shader group texture dictionary.
    RenderModel buildRenderModel(const gamefiles::DrawableBase& drawable);
}
