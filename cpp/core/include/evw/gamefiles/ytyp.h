// Port of CMapTypes / CBaseArchetypeDef (Ytyp) from MetaTypes.cs.
// Ytyp defines archetypes: which drawable/texture dictionaries an object name
// maps to. Combined with Ymap (entity -> archetypeName) this resolves what
// model and textures each placed world object uses.
#pragma once

#include <cstdint>
#include <vector>

#include "evw/gamefiles/meta.h"
#include "evw/gamefiles/metatypes.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
#pragma pack(push, 1)
    // 80-byte map types header.
    struct CMapTypes
    {
        uint32_t unused0;
        uint32_t unused1;
        metatypes::MetaArray extensions;        // 8
        metatypes::MetaArray archetypes;        // 24
        uint32_t name;                          // 40
        uint32_t unused2;
        metatypes::MetaArray dependencies;      // 48
        metatypes::MetaArray compositeEntityTypes; // 64
    };
    static_assert(sizeof(CMapTypes) == 80, "CMapTypes must be 80 bytes");

    // 144-byte base archetype definition.
    struct CBaseArchetypeDef
    {
        uint32_t unused00;
        uint32_t unused01;
        float lodDist;
        uint32_t flags;
        uint32_t specialAttribute;
        uint32_t unused02;
        uint32_t unused03;
        uint32_t unused04;
        math::Vector3 bbMin;
        float unused05;
        math::Vector3 bbMax;
        float unused06;
        math::Vector3 bsCentre;
        float unused07;
        float bsRadius;
        float hdTextureDist;
        uint32_t name;                  // hash
        uint32_t textureDictionary;     // hash
        uint32_t clipDictionary;        // hash
        uint32_t drawableDictionary;    // hash
        uint32_t physicsDictionary;     // hash
        uint32_t assetType;
        uint32_t assetName;             // hash
        uint32_t unused08;
        metatypes::MetaArray extensions; // 120
        uint32_t unused09;
        uint32_t unused10;
    };
    static_assert(sizeof(CBaseArchetypeDef) == 144, "CBaseArchetypeDef must be 144 bytes");
#pragma pack(pop)

    struct YtypFile
    {
        CMapTypes mapTypes{};
        std::vector<CBaseArchetypeDef> archetypes;

        bool loadFromMeta(const Meta& meta, uint32_t cBaseArchetypeDefName);
        bool load(uint32_t systemSize, uint32_t graphicsSize, const std::vector<uint8_t>& data,
                  uint32_t cBaseArchetypeDefName);

        // Finds an archetype by its name hash, or nullptr.
        const CBaseArchetypeDef* findArchetype(uint32_t nameHash) const;
    };
}
