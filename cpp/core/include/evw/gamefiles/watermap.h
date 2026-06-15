// Port of WatermapFile.cs — the 'WMAP' water map (.dat, big-endian) describing
// rivers/lakes/pools over a tile grid. This port parses the header, the per-row
// compression headers, the index/ref tables, and the river/lake/pool structures.
#pragma once

#include <cstdint>
#include <vector>

#include "evw/math/math.h"

namespace evw::gamefiles
{
    struct WatermapCompHeader
    {
        uint8_t start = 0;
        uint8_t count = 0;
        uint16_t offset = 0;
    };

    // A river/lake/pool item. Rivers and lakes carry extra flow fields.
    struct WaterItem
    {
        math::Vector3 position;
        math::Vector3 size;
        uint8_t vectorCount = 0;  // rivers/lakes only
        uint16_t vectorOffset = 0;
    };

    class WatermapFile
    {
    public:
        static constexpr uint32_t MAGIC = 0x574D4150; // 'WMAP'

        // Parses a watermap buffer. Returns false if the magic does not match.
        bool load(const std::vector<uint8_t>& data);

        bool isLoaded() const { return loaded_; }
        uint32_t version() const { return version_; }
        uint16_t width() const { return width_; }
        uint16_t height() const { return height_; }
        math::Vector2 corner() const { return {cornerX_, cornerY_}; }
        math::Vector2 tile() const { return {tileX_, tileY_}; }

        const std::vector<WatermapCompHeader>& compHeaders() const { return compHeaders_; }
        const std::vector<WaterItem>& rivers() const { return rivers_; }
        const std::vector<WaterItem>& lakes() const { return lakes_; }
        const std::vector<WaterItem>& pools() const { return pools_; }

    private:
        bool loaded_ = false;
        uint32_t version_ = 0;
        uint32_t dataLength_ = 0;
        float cornerX_ = 0, cornerY_ = 0, tileX_ = 0, tileY_ = 0;
        uint16_t width_ = 0, height_ = 0;
        uint32_t watermapIndsCount_ = 0, watermapRefsCount_ = 0;
        uint16_t riverVecsCount_ = 0, riverCount_ = 0;
        uint16_t lakeVecsCount_ = 0, lakeCount_ = 0;
        uint16_t poolCount_ = 0, coloursOffset_ = 0;
        std::vector<WatermapCompHeader> compHeaders_;
        std::vector<WaterItem> rivers_;
        std::vector<WaterItem> lakes_;
        std::vector<WaterItem> pools_;
    };
}
