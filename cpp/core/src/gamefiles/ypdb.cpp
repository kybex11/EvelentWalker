#include "evw/gamefiles/ypdb.h"

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    bool YpdbFile::load(const std::vector<uint8_t>& data)
    {
        loaded_ = false;
        if (data.size() < 17) return false;

        DataReader r(data, Endianess::LittleEndian);
        uint8_t magic = r.ReadByte();   // 0x1A = binary (vs text) deserializer marker
        if (magic != 0x1A) return false;

        serializerVersion_ = r.ReadInt32();
        poseMatcherVersion_ = r.ReadInt32();
        signature_ = r.ReadUInt32();
        samplesCount_ = r.ReadInt32();

        loaded_ = true;
        return true;
    }
}
