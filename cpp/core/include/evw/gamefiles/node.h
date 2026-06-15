// Port of Node.cs — the path-node dictionary (Ynd) used for vehicle/ped AI
// routing. Parses the header and the node array; links/junctions are counted
// but not fully parsed yet.
#pragma once

#include <cstdint>
#include <vector>

#include "evw/gamefiles/resource_data.h"

namespace evw::gamefiles
{
#pragma pack(push, 1)
    struct PathNode
    {
        uint32_t unused0, unused1, unused2, unused3;
        uint16_t areaID;
        uint16_t nodeID;
        uint32_t streetName;     // text hash
        uint16_t unused4;
        uint16_t linkID;
        int16_t positionX;
        int16_t positionY;
        uint8_t flags0;
        uint8_t flags1;
        int16_t positionZ;
        uint8_t flags2;
        uint8_t linkCountFlags;
        uint8_t flags3;
        uint8_t flags4;
    };
    static_assert(sizeof(PathNode) == 40, "PathNode must be 40 bytes");
#pragma pack(pop)

    struct NodeDictionary : public ResourceFileBase
    {
        uint32_t nodesCount = 0;
        uint32_t nodesCountVehicle = 0;
        uint32_t nodesCountPed = 0;
        uint32_t linksCount = 0;
        uint32_t junctionsCount = 0;
        std::vector<PathNode> nodes;

        void read(ResourceDataReader& r);
    };
}
