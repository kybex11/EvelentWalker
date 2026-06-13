#include "evw/gamefiles/geometry.h"

namespace evw::gamefiles
{
    void VertexBuffer::read(ResourceDataReader& r)
    {
        r.ReadUInt32();                 // VFT
        r.ReadUInt32();                 // Unknown_4h
        vertexStride = r.ReadUInt16();
        flags = r.ReadUInt16();
        r.ReadUInt32();                 // Unknown_Ch
        dataPointer1 = r.ReadUInt64();
        vertexCount = r.ReadUInt32();
        r.ReadUInt32();                 // Unknown_1Ch
        dataPointer2 = r.ReadUInt64();
        r.ReadUInt64();                 // Unknown_28h
        infoPointer = r.ReadUInt64();
        for (int i = 0; i < 9; ++i) r.ReadUInt64(); // Unknown_38h..78h

        info = r.ReadBlockAt<VertexDeclaration>(infoPointer);
        if (dataPointer1 != 0 && vertexCount != 0 && vertexStride != 0)
            data1 = r.ReadBytesAt(dataPointer1, vertexCount * vertexStride);
        if (dataPointer2 != 0 && vertexCount != 0 && vertexStride != 0)
            data2 = r.ReadBytesAt(dataPointer2, vertexCount * vertexStride);
    }

    void IndexBuffer::read(ResourceDataReader& r)
    {
        r.ReadUInt32();                 // VFT
        r.ReadUInt32();                 // Unknown_4h
        indicesCount = r.ReadUInt32();
        r.ReadUInt32();                 // Unknown_Ch
        indicesPointer = r.ReadUInt64();
        for (int i = 0; i < 9; ++i) r.ReadUInt64(); // Unknown_18h..58h

        indices = r.ReadUshortsAt(indicesPointer, indicesCount);
    }
}
