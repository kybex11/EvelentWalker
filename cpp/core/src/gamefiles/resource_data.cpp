#include "evw/gamefiles/resource_data.h"

#include <algorithm>

namespace evw::gamefiles
{
    ResourceDataReader::ResourceDataReader(uint32_t systemSize, uint32_t graphicsSize,
                                           const std::vector<uint8_t>& data, Endianess endianess)
        : endianess_(endianess)
    {
        size_t sys = std::min<size_t>(systemSize, data.size());
        system_.assign(data.begin(), data.begin() + sys);
        size_t gfxStart = systemSize;
        size_t gfxEnd = std::min<size_t>(static_cast<size_t>(systemSize) + graphicsSize, data.size());
        if (gfxStart < gfxEnd)
            graphics_.assign(data.begin() + gfxStart, data.begin() + gfxEnd);
        position_ = SYSTEM_BASE;
    }

    void ResourceDataReader::readRaw(uint8_t* out, int count, bool ignoreEndianess)
    {
        std::vector<uint8_t>* buf = nullptr;
        uint64_t base = 0;
        if ((position_ & SYSTEM_BASE) == SYSTEM_BASE)
        {
            buf = &system_;
            base = SYSTEM_BASE;
        }
        else if ((position_ & GRAPHICS_BASE) == GRAPHICS_BASE)
        {
            buf = &graphics_;
            base = GRAPHICS_BASE;
        }
        else
        {
            throw std::runtime_error("ResourceDataReader: illegal position");
        }

        uint64_t off = position_ & ~base;
        if (off + count > buf->size())
            throw std::out_of_range("ResourceDataReader: read past end of segment");

        std::memcpy(out, buf->data() + off, static_cast<size_t>(count));
        if (!ignoreEndianess && endianess_ == Endianess::BigEndian)
            std::reverse(out, out + count);

        position_ = (off + count) | base;
    }

    uint8_t ResourceDataReader::ReadByte() { uint8_t v; readRaw(&v, 1); return v; }

    std::vector<uint8_t> ResourceDataReader::ReadBytes(int count)
    {
        std::vector<uint8_t> out(static_cast<size_t>(count));
        if (count > 0) readRaw(out.data(), count, true);
        return out;
    }

    int16_t ResourceDataReader::ReadInt16() { int16_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 2); return v; }
    uint16_t ResourceDataReader::ReadUInt16() { uint16_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 2); return v; }
    int32_t ResourceDataReader::ReadInt32() { int32_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 4); return v; }
    uint32_t ResourceDataReader::ReadUInt32() { uint32_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 4); return v; }
    int64_t ResourceDataReader::ReadInt64() { int64_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 8); return v; }
    uint64_t ResourceDataReader::ReadUInt64() { uint64_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 8); return v; }
    float ResourceDataReader::ReadSingle() { float v; readRaw(reinterpret_cast<uint8_t*>(&v), 4); return v; }

    math::Vector3 ResourceDataReader::ReadVector3()
    {
        math::Vector3 v;
        v.X = ReadSingle(); v.Y = ReadSingle(); v.Z = ReadSingle();
        return v;
    }

    math::Vector4 ResourceDataReader::ReadVector4()
    {
        math::Vector4 v;
        v.X = ReadSingle(); v.Y = ReadSingle(); v.Z = ReadSingle(); v.W = ReadSingle();
        return v;
    }

    std::string ResourceDataReader::ReadString()
    {
        std::string s;
        uint8_t c = ReadByte();
        while (c != 0) { s.push_back(static_cast<char>(c)); c = ReadByte(); }
        return s;
    }

    std::vector<uint8_t> ResourceDataReader::ReadBytesAt(uint64_t position, uint32_t count)
    {
        if (position == 0 || count == 0) return {};
        uint64_t backup = position_;
        position_ = position;
        auto result = ReadBytes(static_cast<int>(count));
        position_ = backup;
        return result;
    }

    std::vector<uint16_t> ResourceDataReader::ReadUshortsAt(uint64_t position, uint32_t count)
    {
        auto bytes = ReadBytesAt(position, count * 2);
        std::vector<uint16_t> result(count);
        if (!bytes.empty()) std::memcpy(result.data(), bytes.data(), count * 2);
        return result;
    }

    std::vector<uint32_t> ResourceDataReader::ReadUintsAt(uint64_t position, uint32_t count)
    {
        auto bytes = ReadBytesAt(position, count * 4);
        std::vector<uint32_t> result(count);
        if (!bytes.empty()) std::memcpy(result.data(), bytes.data(), count * 4);
        return result;
    }

    std::vector<uint64_t> ResourceDataReader::ReadUlongsAt(uint64_t position, uint32_t count)
    {
        auto bytes = ReadBytesAt(position, count * 8);
        std::vector<uint64_t> result(count);
        if (!bytes.empty()) std::memcpy(result.data(), bytes.data(), count * 8);
        return result;
    }

    std::vector<float> ResourceDataReader::ReadFloatsAt(uint64_t position, uint32_t count)
    {
        auto bytes = ReadBytesAt(position, count * 4);
        std::vector<float> result(count);
        if (!bytes.empty()) std::memcpy(result.data(), bytes.data(), count * 4);
        return result;
    }

    std::string ResourceDataReader::ReadStringAt(uint64_t position)
    {
        if (position == 0) return {};
        uint64_t backup = position_;
        position_ = position;
        auto result = ReadString();
        position_ = backup;
        return result;
    }

    // ---- Header blocks ----

    void ResourcePagesInfo::read(ResourceDataReader& r)
    {
        unknown0 = r.ReadUInt32();
        unknown4 = r.ReadUInt32();
        systemPagesCount = r.ReadByte();
        graphicsPagesCount = r.ReadByte();
        unknownA = r.ReadUInt16();
        unknownC = r.ReadUInt32();
        r.setPosition(r.position() + 8ull * (systemPagesCount + graphicsPagesCount));
    }

    void ResourceFileBase::read(ResourceDataReader& r)
    {
        fileVFT = r.ReadUInt32();
        fileUnknown = r.ReadUInt32();
        filePagesInfoPointer = r.ReadUInt64();
        filePagesInfo = r.ReadBlockAt<ResourcePagesInfo>(filePagesInfoPointer);
    }
}
