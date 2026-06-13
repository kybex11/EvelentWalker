#include "evw/gamefiles/ytyp.h"

#include <cstring>

namespace evw::gamefiles
{
    bool YtypFile::loadFromMeta(const Meta& meta, uint32_t cBaseArchetypeDefName)
    {
        const MetaDataBlock* root = meta.rootBlock();
        if (!root || root->data.size() < sizeof(CMapTypes)) return false;

        std::memcpy(&mapTypes, root->data.data(), sizeof(CMapTypes));
        archetypes = metatypes::convertPointerArray<CBaseArchetypeDef>(
            meta, cBaseArchetypeDefName, mapTypes.archetypes);
        return true;
    }

    bool YtypFile::load(uint32_t systemSize, uint32_t graphicsSize, const std::vector<uint8_t>& data,
                        uint32_t cBaseArchetypeDefName)
    {
        ResourceDataReader reader(systemSize, graphicsSize, data);
        Meta meta;
        meta.read(reader);
        return loadFromMeta(meta, cBaseArchetypeDefName);
    }

    const CBaseArchetypeDef* YtypFile::findArchetype(uint32_t nameHash) const
    {
        for (const auto& a : archetypes)
            if (a.name == nameHash) return &a;
        return nullptr;
    }
}
