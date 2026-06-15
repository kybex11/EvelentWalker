#include "evw/gamefiles/node.h"

namespace evw::gamefiles
{
    void NodeDictionary::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        uint64_t nodesPointer = r.ReadUInt64();
        nodesCount = r.ReadUInt32();
        nodesCountVehicle = r.ReadUInt32();
        nodesCountPed = r.ReadUInt32();
        r.ReadUInt32();                 // Unk24
        r.ReadUInt64();                 // LinksPtr
        linksCount = r.ReadUInt32();
        r.ReadUInt32();                 // Unk34
        r.ReadUInt64();                 // JunctionsPtr
        r.ReadUInt64();                 // JunctionHeightmapBytesPtr
        r.ReadUInt32();                 // Unk48
        r.ReadUInt32();                 // Unk4C
        r.ReadUInt64();                 // JunctionRefsPtr
        r.ReadUInt16();                 // JunctionRefsCount0
        r.ReadUInt16();                 // JunctionRefsCount1
        r.ReadUInt32();                 // Unk5C
        junctionsCount = r.ReadUInt32();
        r.ReadUInt32();                 // JunctionHeightmapBytesCount
        r.ReadUInt32();                 // Unk68
        r.ReadUInt32();                 // Unk6C

        nodes = r.ReadStructsAt<PathNode>(nodesPointer, nodesCount);
    }
}
