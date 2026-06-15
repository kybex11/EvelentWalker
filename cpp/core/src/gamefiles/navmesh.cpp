#include "evw/gamefiles/navmesh.h"

namespace evw::gamefiles
{
    void NavMesh::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        contentFlags = r.ReadUInt32();
        r.ReadUInt32();                 // VersionUnk1
        r.ReadUInt32();                 // Unused_018h
        r.ReadUInt32();                 // Unused_01Ch
        for (int i = 0; i < 16; ++i) r.ReadSingle(); // Transform (4x4)
        aabbSize = r.ReadVector3();
        r.ReadUInt32();                 // AABBUnk
        r.ReadUInt64();                 // VerticesPointer
        r.ReadUInt32(); r.ReadUInt32(); // Unused_078h/07Ch
        r.ReadUInt64();                 // IndicesPointer
        r.ReadUInt64();                 // EdgesPointer
        r.ReadUInt32();                 // EdgesIndicesCount
        for (int i = 0; i < 16; ++i) r.ReadUInt32(); // AdjAreaIDs (NavMeshUintArray, 64 bytes)
        r.ReadUInt64();                 // PolysPointer
        r.ReadUInt64();                 // SectorTreePointer
        r.ReadUInt64();                 // PortalsPointer
        r.ReadUInt64();                 // PortalLinksPointer
        verticesCount = r.ReadUInt32();
        polysCount = r.ReadUInt32();
        areaID = r.ReadUInt32();
        totalBytes = r.ReadUInt32();
        pointsCount = r.ReadUInt32();
        r.ReadUInt32();                 // PortalsCount
        r.ReadUInt32();                 // PortalLinksCount
        r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32(); // Unused_154/158/15C
    }
}
