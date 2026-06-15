// Headers for the name-keyed pgDictionary resources that share a common layout:
//   ResourceFileBase + leading unknowns + SimpleList64<hash> + PointerList64<T>.
// Ported at header level (name hashes + item count); the per-item payloads
// (Expression / crFrameFilter / characterCloth) are not fully parsed yet.
//   Yed = ExpressionDictionary, Yfd = FrameFilterDictionary, Yld = ClothDictionary
#pragma once

#include <cstdint>
#include <vector>

#include "evw/gamefiles/resource_data.h"

namespace evw::gamefiles
{
    struct ExpressionDictionary : public ResourceFileBase
    {
        std::vector<uint32_t> nameHashes;
        uint16_t itemCount = 0;
        void read(ResourceDataReader& r);
    };

    struct FrameFilterDictionary : public ResourceFileBase
    {
        std::vector<uint32_t> nameHashes;
        uint16_t itemCount = 0;
        void read(ResourceDataReader& r);
    };

    struct ClothDictionary : public ResourceFileBase
    {
        std::vector<uint32_t> nameHashes;
        uint16_t itemCount = 0;
        void read(ResourceDataReader& r);
    };
}
