// Port of DrawableGeometry / DrawableModel from Drawable.cs (legacy read path).
// A model is a list of geometries; each geometry binds a VertexBuffer + IndexBuffer
// plus a shader id and bounding box. This is the mesh layer of Ydr/Ydd/Yft.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "evw/gamefiles/geometry.h"
#include "evw/gamefiles/resource_data.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
    // 32-byte axis-aligned bounding box (Min/Max as Vector4).
    struct AABB_s
    {
        math::Vector4 min;
        math::Vector4 max;
    };

    struct DrawableGeometry
    {
        uint64_t vertexBufferPointer = 0;
        uint64_t indexBufferPointer = 0;
        uint32_t indicesCount = 0;
        uint32_t trianglesCount = 0;
        uint16_t verticesCount = 0;
        uint16_t vertexStride = 0;
        uint64_t boneIdsPointer = 0;
        uint16_t boneIdsCount = 0;

        std::shared_ptr<VertexBuffer> vertexBuffer;
        std::shared_ptr<IndexBuffer> indexBuffer;
        std::vector<uint16_t> boneIds;

        uint16_t shaderId = 0;   // assigned by parent model
        AABB_s aabb{};           // assigned by parent model

        void read(ResourceDataReader& r);
    };

    struct DrawableModel
    {
        uint64_t geometriesPointer = 0;
        uint16_t geometriesCount = 0;
        uint64_t boundsPointer = 0;
        uint64_t shaderMappingPointer = 0;
        uint32_t skeletonBinding = 0;
        uint16_t renderMaskFlags = 0;

        std::vector<uint16_t> shaderMapping;
        std::vector<uint64_t> geometryPointers;
        std::vector<AABB_s> boundsData;
        std::vector<std::shared_ptr<DrawableGeometry>> geometries;

        void read(ResourceDataReader& r);
    };
}
