// Port of CacheDatFile.cs (read path) — the gta cache_y*.dat file. It is a
// text-tagged container: a version string, a <fileDates> block, and binary
// "modules" (fwMapDataStore / CInteriorProxy / BoundsStore). This port reads the
// version, the file-date lines, and the fixed-size BoundsStore items.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "evw/math/math.h"

namespace evw::gamefiles
{
    struct BoundsStoreItem
    {
        uint32_t name = 0;
        math::Vector3 min;
        math::Vector3 max;
        uint32_t layer = 0;
    };

    class CacheDatFile
    {
    public:
        // Parses a cache .dat buffer. Returns false if empty.
        bool load(const std::vector<uint8_t>& data);

        bool isLoaded() const { return loaded_; }
        const std::string& version() const { return version_; }
        const std::vector<std::string>& fileDates() const { return fileDates_; }
        const std::vector<BoundsStoreItem>& boundsStore() const { return boundsStore_; }
        uint32_t mapDataStoreLength() const { return mapDataStoreLength_; }
        uint32_t interiorProxyCount() const { return interiorProxyCount_; }

    private:
        bool loaded_ = false;
        std::string version_;
        std::vector<std::string> fileDates_;
        std::vector<BoundsStoreItem> boundsStore_;
        uint32_t mapDataStoreLength_ = 0;
        uint32_t interiorProxyCount_ = 0;
    };
}
