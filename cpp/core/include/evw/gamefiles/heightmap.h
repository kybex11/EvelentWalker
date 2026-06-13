// Port of HeightmapFile.cs — the terrain heightmap (.dat). Stores per-cell
// min/max height bytes, optionally run-length compressed per row.
#pragma once

#include <cstdint>
#include <vector>

#include "evw/math/math.h"

namespace evw::gamefiles
{
    struct HeightmapCompHeader
    {
        uint16_t start = 0;
        uint16_t count = 0;
        uint32_t dataOffset = 0;
    };

    class HeightmapFile
    {
    public:
        bool load(const std::vector<uint8_t>& data);

        uint32_t magic = 0;
        uint32_t compressed = 0;
        uint16_t width = 0;
        uint16_t height = 0;
        math::Vector3 bbMin;
        math::Vector3 bbMax;
        std::vector<uint8_t> maxHeights;
        std::vector<uint8_t> minHeights;
    };
}
