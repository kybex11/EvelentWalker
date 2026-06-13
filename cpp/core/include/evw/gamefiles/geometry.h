// Port of the geometry buffers from EvelentWalker.Core/GameFiles/Resources/Drawable.cs
// (legacy / non-gen9 read path): VertexDeclaration, VertexBuffer, IndexBuffer.
// These are the reusable containers underlying Ydr/Ydd/Yft drawables.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "evw/gamefiles/resource_data.h"

namespace evw::gamefiles
{
    // 16-byte descriptor of a vertex layout.
    struct VertexDeclaration
    {
        uint32_t flags = 0;
        uint16_t stride = 0;
        uint8_t unknown6 = 0;
        uint8_t count = 0;
        uint64_t types = 0;

        void read(ResourceDataReader& r)
        {
            flags = r.ReadUInt32();
            stride = r.ReadUInt16();
            unknown6 = r.ReadByte();
            count = r.ReadByte();
            types = r.ReadUInt64();
        }
    };

    // 128-byte header referencing vertex data + a declaration.
    struct VertexBuffer
    {
        uint16_t vertexStride = 0;
        uint16_t flags = 0;
        uint32_t vertexCount = 0;
        uint64_t dataPointer1 = 0;
        uint64_t dataPointer2 = 0;
        uint64_t infoPointer = 0;

        std::shared_ptr<VertexDeclaration> info;
        std::vector<uint8_t> data1; // raw vertex bytes (vertexCount * vertexStride)
        std::vector<uint8_t> data2;

        void read(ResourceDataReader& r);
    };

    // 96-byte header referencing a 16-bit index array.
    struct IndexBuffer
    {
        uint32_t indicesCount = 0;
        uint64_t indicesPointer = 0;
        std::vector<uint16_t> indices;

        void read(ResourceDataReader& r);
    };
}
