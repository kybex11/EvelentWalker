#include "evw/gamefiles/awc.h"

#include <cstring>

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    bool AwcFile::load(const std::vector<uint8_t>& data)
    {
        if (data.size() < 16) return false;

        uint32_t m;
        std::memcpy(&m, data.data(), 4);
        if (m != MAGIC && m != MAGIC_BE) return false;

        // Big-endian variant uses reversed byte order for the whole file.
        Endianess endian = (m == MAGIC) ? Endianess::LittleEndian : Endianess::BigEndian;
        DataReader r(data, endian);

        magic = r.ReadUInt32();
        version = r.ReadUInt16();
        flags = r.ReadUInt16();
        streamCount = r.ReadInt32();
        dataOffset = r.ReadInt32();

        if (chunkIndicesFlag() && streamCount > 0)
        {
            chunkIndices.resize(streamCount);
            for (int i = 0; i < streamCount; ++i) chunkIndices[i] = r.ReadUInt16();
        }
        return true;
    }
}
