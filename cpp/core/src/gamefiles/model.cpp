#include "evw/gamefiles/model.h"

namespace evw::gamefiles
{
    void DrawableGeometry::read(ResourceDataReader& r)
    {
        r.ReadUInt32();                       // VFT
        r.ReadUInt32();                       // Unknown_4h
        r.ReadUInt64();                       // Unknown_8h
        r.ReadUInt64();                       // Unknown_10h
        vertexBufferPointer = r.ReadUInt64();
        r.ReadUInt64();                       // Unknown_20h
        r.ReadUInt64();                       // Unknown_28h
        r.ReadUInt64();                       // Unknown_30h
        indexBufferPointer = r.ReadUInt64();
        r.ReadUInt64();                       // Unknown_40h
        r.ReadUInt64();                       // Unknown_48h
        r.ReadUInt64();                       // Unknown_50h
        indicesCount = r.ReadUInt32();
        trianglesCount = r.ReadUInt32();
        verticesCount = r.ReadUInt16();
        r.ReadUInt16();                       // Unknown_62h
        r.ReadUInt32();                       // Unknown_64h
        boneIdsPointer = r.ReadUInt64();
        vertexStride = r.ReadUInt16();
        boneIdsCount = r.ReadUInt16();
        r.ReadUInt32();                       // Unknown_74h
        r.ReadUInt64();                       // VertexDataPointer
        r.ReadUInt64();                       // Unknown_80h
        r.ReadUInt64();                       // Unknown_88h
        r.ReadUInt64();                       // Unknown_90h

        vertexBuffer = r.ReadBlockAt<VertexBuffer>(vertexBufferPointer);
        indexBuffer = r.ReadBlockAt<IndexBuffer>(indexBufferPointer);
        boneIds = r.ReadUshortsAt(boneIdsPointer, boneIdsCount);
    }

    void DrawableModel::read(ResourceDataReader& r)
    {
        r.ReadUInt32();                       // VFT
        r.ReadUInt32();                       // Unknown_4h
        geometriesPointer = r.ReadUInt64();
        geometriesCount = r.ReadUInt16();
        r.ReadUInt16();                       // GeometriesCount2
        r.ReadUInt32();                       // Unknown_14h
        boundsPointer = r.ReadUInt64();
        shaderMappingPointer = r.ReadUInt64();
        skeletonBinding = r.ReadUInt32();
        renderMaskFlags = r.ReadUInt16();
        r.ReadUInt16();                       // GeometriesCount3

        shaderMapping = r.ReadUshortsAt(shaderMappingPointer, geometriesCount);
        geometryPointers = r.ReadUlongsAt(geometriesPointer, geometriesCount);
        uint32_t boundsCount = geometriesCount > 1 ? geometriesCount + 1u : geometriesCount;
        boundsData = r.ReadStructsAt<AABB_s>(boundsPointer, boundsCount);

        geometries.clear();
        geometries.reserve(geometryPointers.size());
        for (uint64_t ptr : geometryPointers)
            geometries.push_back(r.ReadBlockAt<DrawableGeometry>(ptr));

        for (size_t i = 0; i < geometries.size(); ++i)
        {
            auto& geom = geometries[i];
            if (!geom) continue;
            geom->shaderId = (i < shaderMapping.size()) ? shaderMapping[i] : 0;
            if (!boundsData.empty())
                geom->aabb = (boundsData.size() > 1 && (i + 1) < boundsData.size()) ? boundsData[i + 1]
                                                                                     : boundsData[0];
        }
    }

    void Skeleton::read(ResourceDataReader& r)
    {
        r.ReadUInt32();                 // VFT
        r.ReadUInt32();                 // Unknown_4h
        r.ReadUInt64();                 // Unknown_8h
        boneTagsPointer = r.ReadUInt64();
        r.ReadUInt16();                 // BoneTagsCapacity
        boneTagsCount = r.ReadUInt16();
        r.ReadUInt32();                 // Unknown_1Ch
        bonesPointer = r.ReadUInt64();
        uint64_t transformsInvPtr = r.ReadUInt64();
        uint64_t transformsPtr = r.ReadUInt64();
        uint64_t parentIndicesPtr = r.ReadUInt64();
        uint64_t childIndicesPtr = r.ReadUInt64();
        r.ReadUInt64();                 // Unknown_48h
        r.ReadUInt32();                 // Unknown_50h
        r.ReadUInt32();                 // Unknown_54h
        r.ReadUInt32();                 // Unknown_58h
        r.ReadUInt16();                 // Unknown_5Ch
        bonesCount = r.ReadUInt16();
        childIndicesCount = r.ReadUInt16();
        r.ReadUInt16();                 // Unknown_62h
        r.ReadUInt32();                 // Unknown_64h
        r.ReadUInt64();                 // Unknown_68h

        transformationsInverted = r.ReadStructsAt<math::Matrix>(transformsInvPtr, bonesCount);
        transformations = r.ReadStructsAt<math::Matrix>(transformsPtr, bonesCount);
        parentIndices = r.ReadShortsAt(parentIndicesPtr, bonesCount);
        childIndices = r.ReadShortsAt(childIndicesPtr, childIndicesCount);
        // BoneTags and the bones block are not parsed yet.
    }
}