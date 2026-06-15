#include "evw/gamefiles/watermap.h"

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    namespace
    {
        // Reads the 32-byte WaterItem base (position, unk, size, unk).
        WaterItem readItemBase(DataReader& r)
        {
            WaterItem it;
            it.position = r.ReadVector3();
            r.ReadUInt32();           // Unk04
            it.size = r.ReadVector3();
            r.ReadUInt32();           // Unk09
            return it;
        }

        // Reads a river/lake flow item (32-byte base + 16-byte flow fields).
        WaterItem readFlow(DataReader& r)
        {
            WaterItem it = readItemBase(r);
            it.vectorCount = r.ReadByte();
            r.ReadByte();             // Unk11
            it.vectorOffset = r.ReadUInt16();
            r.ReadUInt32();           // Unk13
            r.ReadUInt32();           // Unk14
            r.ReadUInt32();           // Unk15
            return it;
        }
    }

    bool WatermapFile::load(const std::vector<uint8_t>& data)
    {
        loaded_ = false;
        DataReader r(data, Endianess::BigEndian);
        if (r.length() < 56) return false;

        if (r.ReadUInt32() != MAGIC) return false;
        version_ = r.ReadUInt32();
        dataLength_ = r.ReadUInt32();
        cornerX_ = r.ReadSingle();
        cornerY_ = r.ReadSingle();
        tileX_ = r.ReadSingle();
        tileY_ = r.ReadSingle();
        width_ = r.ReadUInt16();
        height_ = r.ReadUInt16();
        watermapIndsCount_ = r.ReadUInt32();
        watermapRefsCount_ = r.ReadUInt32();
        riverVecsCount_ = r.ReadUInt16();
        riverCount_ = r.ReadUInt16();
        lakeVecsCount_ = r.ReadUInt16();
        lakeCount_ = r.ReadUInt16();
        poolCount_ = r.ReadUInt16();
        coloursOffset_ = r.ReadUInt16();
        r.ReadBytes(8);               // Unks1 flags

        compHeaders_.resize(height_);
        for (uint16_t i = 0; i < height_; ++i)
        {
            WatermapCompHeader h;
            h.start = r.ReadByte();
            h.count = r.ReadByte();
            h.offset = r.ReadUInt16();
            compHeaders_[i] = h;
        }

        // Index table (int16) and ref table (uint16).
        r.setPosition(r.position() + static_cast<int64_t>(watermapIndsCount_) * 2);
        r.setPosition(r.position() + static_cast<int64_t>(watermapRefsCount_) * 2);

        // Align the river/lake/pool block to 16 bytes (matches the original).
        int64_t shortslen = static_cast<int64_t>(watermapIndsCount_ + watermapRefsCount_) * 2 +
                            static_cast<int64_t>(height_) * 4;
        int64_t padcount = (16 - (shortslen % 16)) % 16;
        r.setPosition(r.position() + padcount);

        // River vectors then river flow items.
        r.setPosition(r.position() + static_cast<int64_t>(riverVecsCount_) * 16);
        rivers_.clear();
        rivers_.reserve(riverCount_);
        for (uint16_t i = 0; i < riverCount_; ++i) rivers_.push_back(readFlow(r));

        // Lake vectors then lake flow items.
        r.setPosition(r.position() + static_cast<int64_t>(lakeVecsCount_) * 16);
        lakes_.clear();
        lakes_.reserve(lakeCount_);
        for (uint16_t i = 0; i < lakeCount_; ++i) lakes_.push_back(readFlow(r));

        // Pools (base item only).
        pools_.clear();
        pools_.reserve(poolCount_);
        for (uint16_t i = 0; i < poolCount_; ++i) pools_.push_back(readItemBase(r));

        loaded_ = true;
        return true;
    }
}
