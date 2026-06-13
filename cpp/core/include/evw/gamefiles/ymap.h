// Port of CMapData / CEntityDef (Ymap) from MetaTypes.cs, plus a small loader
// that reads the entity list from a parsed Meta. Ymap describes placement of
// entities (props/buildings) in the world.
#pragma once

#include <cstdint>
#include <vector>

#include "evw/gamefiles/meta.h"
#include "evw/gamefiles/metatypes.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
#pragma pack(push, 1)
    // 128-byte entity definition.
    struct CEntityDef
    {
        uint32_t unused0;
        uint32_t unused1;
        uint32_t archetypeName;     // hash
        uint32_t flags;
        uint32_t guid;
        uint32_t unused2;
        uint32_t unused3;
        uint32_t unused4;
        math::Vector3 position;     // 12 bytes
        float unused5;
        math::Vector4 rotation;     // 16 bytes
        float scaleXY;
        float scaleZ;
        int32_t parentIndex;
        float lodDist;
        float childLodDist;
        uint32_t lodLevel;
        uint32_t numChildren;
        uint32_t priorityLevel;
        metatypes::MetaArray extensions; // 16 bytes
        int32_t ambientOcclusionMultiplier;
        int32_t artificialAmbientOcclusion;
        uint32_t tintValue;
        uint32_t unused6;
    };
    static_assert(sizeof(CEntityDef) == 128, "CEntityDef must be 128 bytes");

    // 512-byte map data. Fields beyond carGenerators (lights/block) are padded.
    struct CMapData
    {
        uint32_t unused0;
        uint32_t unused1;
        uint32_t name;              // hash
        uint32_t parent;            // hash
        uint32_t flags;
        uint32_t contentFlags;
        uint32_t unused2;
        uint32_t unused3;
        math::Vector3 streamingExtentsMin;
        float unused4;
        math::Vector3 streamingExtentsMax;
        float unused5;
        math::Vector3 entitiesExtentsMin;
        float unused6;
        math::Vector3 entitiesExtentsMax;
        float unused7;
        metatypes::MetaArray entities;          // 96
        metatypes::MetaArray containerLods;     // 112
        metatypes::MetaArray boxOccluders;      // 128
        metatypes::MetaArray occludeModels;     // 144
        metatypes::MetaArray physicsDictionaries; // 160
        uint8_t instancedData[48];              // 176 (rage__fwInstancedMapData)
        metatypes::MetaArray timeCycleModifiers; // 224
        metatypes::MetaArray carGenerators;     // 240
        uint8_t lightsAndBlock[256];            // 256..512 (LODLights+DistantLODLights+block)
    };
    static_assert(sizeof(CMapData) == 512, "CMapData must be 512 bytes");
#pragma pack(pop)

    // Parsed Ymap: the map data header and its entities.
    struct YmapFile
    {
        CMapData mapData{};
        std::vector<CEntityDef> entities;

        // Loads from a parsed Meta. `cEntityDefName` is the structure name hash of
        // CEntityDef in this file (the entities pointer-array element type).
        bool loadFromMeta(const Meta& meta, uint32_t cEntityDefName);

        // Convenience: parses a Meta resource then loads.
        bool load(uint32_t systemSize, uint32_t graphicsSize, const std::vector<uint8_t>& data,
                  uint32_t cEntityDefName);
    };
}
