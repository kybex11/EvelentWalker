#include "evw/gamefiles/geometry.h"

#include <cstring>

#include "evw/math/math.h"

namespace evw::gamefiles
{
    namespace
    {
        // IEEE half (16-bit) to float.
        float halfToFloat(uint16_t h)
        {
            uint32_t sign = (h >> 15) & 1;
            uint32_t exp = (h >> 10) & 0x1F;
            uint32_t mant = h & 0x3FF;
            uint32_t f;
            if (exp == 0)
            {
                if (mant == 0) { f = sign << 31; }
                else
                {
                    exp = 127 - 15 + 1;
                    while ((mant & 0x400) == 0) { mant <<= 1; --exp; }
                    mant &= 0x3FF;
                    f = (sign << 31) | (exp << 23) | (mant << 13);
                }
            }
            else if (exp == 0x1F)
            {
                f = (sign << 31) | (0xFF << 23) | (mant << 13);
            }
            else
            {
                f = (sign << 31) | ((exp - 15 + 127) << 23) | (mant << 13);
            }
            float result;
            std::memcpy(&result, &f, 4);
            return result;
        }
    }

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

    std::vector<math::Vector3> VertexBuffer::extractPositions() const
    {
        std::vector<math::Vector3> positions;
        if (!info || data1.empty() || vertexStride == 0 || vertexCount == 0) return positions;
        if (!info->hasComponent(0)) return positions; // no position semantic

        VertexComponentType ct = info->componentType(0);
        int off = info->componentOffset(0);
        positions.reserve(vertexCount);

        for (uint32_t v = 0; v < vertexCount; ++v)
        {
            size_t base = static_cast<size_t>(v) * vertexStride + off;
            math::Vector3 p;
            if (ct == VertexComponentType::Float3 || ct == VertexComponentType::Float4)
            {
                if (base + 12 > data1.size()) break;
                std::memcpy(&p.X, data1.data() + base + 0, 4);
                std::memcpy(&p.Y, data1.data() + base + 4, 4);
                std::memcpy(&p.Z, data1.data() + base + 8, 4);
            }
            else if (ct == VertexComponentType::Half4 || ct == VertexComponentType::Half2)
            {
                if (base + 6 > data1.size()) break;
                uint16_t hx, hy, hz;
                std::memcpy(&hx, data1.data() + base + 0, 2);
                std::memcpy(&hy, data1.data() + base + 2, 2);
                std::memcpy(&hz, data1.data() + base + 4, 2);
                p = {halfToFloat(hx), halfToFloat(hy), halfToFloat(hz)};
            }
            else
            {
                break; // unsupported position format
            }
            positions.push_back(p);
        }
        return positions;
    }

    std::vector<math::Vector3> VertexBuffer::extractVec3(int semantic) const
    {
        std::vector<math::Vector3> out;
        if (!info || data1.empty() || vertexStride == 0 || vertexCount == 0) return out;
        if (!info->hasComponent(semantic)) return out;

        VertexComponentType ct = info->componentType(semantic);
        int off = info->componentOffset(semantic);
        out.reserve(vertexCount);
        for (uint32_t v = 0; v < vertexCount; ++v)
        {
            size_t base = static_cast<size_t>(v) * vertexStride + off;
            math::Vector3 p;
            if (ct == VertexComponentType::Float3 || ct == VertexComponentType::Float4)
            {
                if (base + 12 > data1.size()) break;
                std::memcpy(&p.X, data1.data() + base + 0, 4);
                std::memcpy(&p.Y, data1.data() + base + 4, 4);
                std::memcpy(&p.Z, data1.data() + base + 8, 4);
            }
            else if (ct == VertexComponentType::Half4 || ct == VertexComponentType::Half2)
            {
                if (base + 6 > data1.size()) break;
                uint16_t hx, hy, hz;
                std::memcpy(&hx, data1.data() + base + 0, 2);
                std::memcpy(&hy, data1.data() + base + 2, 2);
                std::memcpy(&hz, data1.data() + base + 4, 2);
                p = {halfToFloat(hx), halfToFloat(hy), halfToFloat(hz)};
            }
            else break;
            out.push_back(p);
        }
        return out;
    }

    std::vector<math::Vector3> VertexBuffer::extractNormals() const { return extractVec3(3); }

    std::vector<math::Vector2> VertexBuffer::extractTexCoords0() const
    {
        std::vector<math::Vector2> out;
        if (!info || data1.empty() || vertexStride == 0 || vertexCount == 0) return out;
        const int semantic = 6;
        if (!info->hasComponent(semantic)) return out;

        VertexComponentType ct = info->componentType(semantic);
        int off = info->componentOffset(semantic);
        out.reserve(vertexCount);
        for (uint32_t v = 0; v < vertexCount; ++v)
        {
            size_t base = static_cast<size_t>(v) * vertexStride + off;
            math::Vector2 uv;
            if (ct == VertexComponentType::Float2 || ct == VertexComponentType::Float3 ||
                ct == VertexComponentType::Float4)
            {
                if (base + 8 > data1.size()) break;
                std::memcpy(&uv.X, data1.data() + base + 0, 4);
                std::memcpy(&uv.Y, data1.data() + base + 4, 4);
            }
            else if (ct == VertexComponentType::Half2 || ct == VertexComponentType::Half4)
            {
                if (base + 4 > data1.size()) break;
                uint16_t hx, hy;
                std::memcpy(&hx, data1.data() + base + 0, 2);
                std::memcpy(&hy, data1.data() + base + 2, 2);
                uv = {halfToFloat(hx), halfToFloat(hy)};
            }
            else break;
            out.push_back(uv);
        }
        return out;
    }

    std::vector<uint32_t> VertexBuffer::extractColours0() const
    {
        std::vector<uint32_t> out;
        if (!info || data1.empty() || vertexStride == 0 || vertexCount == 0) return out;
        const int semantic = 4;
        if (!info->hasComponent(semantic)) return out;

        VertexComponentType ct = info->componentType(semantic);
        if (ct != VertexComponentType::Colour && ct != VertexComponentType::UByte4 &&
            ct != VertexComponentType::RGBA8SNorm)
            return out;
        int off = info->componentOffset(semantic);
        out.reserve(vertexCount);
        for (uint32_t v = 0; v < vertexCount; ++v)
        {
            size_t base = static_cast<size_t>(v) * vertexStride + off;
            if (base + 4 > data1.size()) break;
            uint32_t c;
            std::memcpy(&c, data1.data() + base, 4);
            out.push_back(c);
        }
        return out;
    }
}
