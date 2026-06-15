// Port of the AwcFile header from AwcFile.cs — the audio wave container (.awc).
// Parses the header and per-stream chunk indices; full stream/chunk decoding and
// the whole-file RSXXTEA decryption are not ported yet.
#pragma once

#include <cstdint>
#include <vector>

namespace evw::gamefiles
{
    class AwcFile
    {
    public:
        bool load(const std::vector<uint8_t>& data);

        uint32_t magic = 0;
        uint16_t version = 0;
        uint16_t flags = 0;
        int32_t streamCount = 0;
        int32_t dataOffset = 0;
        std::vector<uint16_t> chunkIndices;

        bool chunkIndicesFlag() const { return (flags & 1) == 1; }
        bool multiChannelFlag() const { return (flags & 4) == 4; }

        static constexpr uint32_t MAGIC = 0x54414441;       // "ADAT" little-endian
        static constexpr uint32_t MAGIC_BE = 0x41444154;    // big-endian variant
    };
}
