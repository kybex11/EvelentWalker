#include "evw/gamefiles/mrf.h"

namespace evw::gamefiles
{
    bool MrfFile::load(const std::vector<uint8_t>& data)
    {
        loaded_ = false;
        if (data.size() < 32) return false;

        DataReader r(data, Endianess::LittleEndian);

        magic_ = r.ReadUInt32();        // 'MoVE'
        version_ = r.ReadUInt32();      // GTA5 = 2
        headerUnk1_ = r.ReadUInt32();   // 0
        headerUnk2_ = r.ReadUInt32();
        headerUnk3_ = r.ReadUInt32();   // 0
        dataLength_ = r.ReadUInt32();
        unkBytesCount_ = r.ReadUInt32();

        if (magic_ != MAGIC || version_ != 2 || headerUnk1_ != 0 || headerUnk2_ != 0)
            return false;

        // Unk1 block (unused in the final game). Each item is a size-prefixed
        // byte blob; we skip over the bytes after recording the count.
        unk1Count_ = r.ReadUInt32();
        for (uint32_t i = 0; i < unk1Count_; ++i)
        {
            uint32_t size = r.ReadUInt32();
            r.setPosition(r.position() + static_cast<int64_t>(size));
        }

        uint32_t triggerCount = r.ReadUInt32();
        triggers_.clear();
        triggers_.reserve(triggerCount);
        for (uint32_t i = 0; i < triggerCount; ++i)
        {
            MrfMoveNetworkBit b;
            b.name = r.ReadUInt32();
            b.bitPosition = r.ReadUInt32();
            triggers_.push_back(b);
        }

        uint32_t flagCount = r.ReadUInt32();
        flags_.clear();
        flags_.reserve(flagCount);
        for (uint32_t i = 0; i < flagCount; ++i)
        {
            MrfMoveNetworkBit b;
            b.name = r.ReadUInt32();
            b.bitPosition = r.ReadUInt32();
            flags_.push_back(b);
        }

        loaded_ = true;
        return true;
    }
}
