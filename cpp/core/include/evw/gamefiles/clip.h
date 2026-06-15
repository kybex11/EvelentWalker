// Port of Clip.cs header — the clip dictionary (.ycd) resource holding animation
// clips. This port parses the ClipDictionary structure header (offsets, capacity,
// entry count) and the clip pointer array; the per-clip and AnimationMap payloads
// are not fully parsed yet.
#pragma once

#include <cstdint>
#include <vector>

#include "evw/gamefiles/resource_data.h"

namespace evw::gamefiles
{
    struct ClipDictionary : public ResourceFileBase
    {
        uint32_t unknown10 = 0;
        uint32_t unknown14 = 0;
        uint64_t animationsPointer = 0;
        uint32_t unknown20 = 0x00000101;
        uint32_t unknown24 = 0;
        uint64_t clipsPointer = 0;
        uint16_t clipsMapCapacity = 0;
        uint16_t clipsMapEntries = 0;
        uint32_t unknown34 = 0x01000000;
        uint32_t unknown38 = 0;
        uint32_t unknown3C = 0;

        // Pointers to each clip-map entry (capacity-sized slot table).
        std::vector<uint64_t> clipPointers;

        void read(ResourceDataReader& r);
    };
}
