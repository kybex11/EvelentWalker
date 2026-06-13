#include "evw/gamefiles/ymap.h"

#include <cstring>

namespace evw::gamefiles
{
    bool YmapFile::loadFromMeta(const Meta& meta, uint32_t cEntityDefName)
    {
        const MetaDataBlock* root = meta.rootBlock();
        if (!root || root->data.size() < sizeof(CMapData)) return false;

        std::memcpy(&mapData, root->data.data(), sizeof(CMapData));

        entities = metatypes::convertPointerArray<CEntityDef>(meta, cEntityDefName, mapData.entities);
        return true;
    }

    bool YmapFile::load(uint32_t systemSize, uint32_t graphicsSize, const std::vector<uint8_t>& data,
                        uint32_t cEntityDefName)
    {
        ResourceDataReader reader(systemSize, graphicsSize, data);
        Meta meta;
        meta.read(reader);
        return loadFromMeta(meta, cEntityDefName);
    }
}
