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
    enum class VertexComponentType : uint8_t
    {
        Nothing = 0, Half2 = 1, Float = 2, Half4 = 3, FloatUnk = 4,
        Float2 = 5, Float3 = 6, Float4 = 7, UByte4 = 8, Colour = 9, RGBA8SNorm = 10,
    };

    inline int vertexComponentSize(VertexComponentType t)
    {
        switch (t)
        {
        case VertexComponentType::Half2: return 4;
        case VertexComponentType::Float: return 4;
        case VertexComponentType::Half4: return 8;
        case VertexComponentType::Float2: return 8;
        case VertexComponentType::Float3: return 12;
        case VertexComponentType::Float4: return 16;
        case VertexComponentType::UByte4: return 4;
        case VertexComponentType::Colour: return 4;
        case VertexComponentType::RGBA8SNorm: return 4;
        default: return 0;
        }
    }

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

        bool hasComponent(int semanticIndex) const { return ((flags >> semanticIndex) & 1) != 0; }

        VertexComponentType componentType(int semanticIndex) const
        {
            return static_cast<VertexComponentType>((types >> (semanticIndex * 4)) & 0xF);
        }

        // Byte offset of the given semantic slot within a vertex.
        int componentOffset(int semanticIndex) const
        {
            int offset = 0;
            for (int k = 0; k < semanticIndex; ++k)
                if (((flags >> k) & 1) != 0)
                    offset += vertexComponentSize(componentType(k));
            return offset;
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

        // Extracts per-vertex XYZ positions (semantic slot 0) using the layout.
        // Returns empty if there is no position data.
        std::vector<math::Vector3> extractPositions() const;
        // Normals (semantic slot 3), as XYZ.
        std::vector<math::Vector3> extractNormals() const;
        // Primary texture coordinates (semantic slot 6), as UV.
        std::vector<math::Vector2> extractTexCoords0() const;
        // Primary vertex colours (semantic slot 4), as packed RGBA8.
        std::vector<uint32_t> extractColours0() const;

    private:
        std::vector<math::Vector3> extractVec3(int semantic) const;
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
