// Port of DistantLightsFile.cs — the distant-lights grid (.dat, big-endian)
// describing streamed night-time light paths. Two variants exist: HD
// (grid 32, 1024 cells) and non-HD (grid 16, 256 cells), selected by filename.
#pragma once

#include <cstdint>
#include <vector>

namespace evw::gamefiles
{
    struct DistantLightsNode
    {
        int16_t x = 0;
        int16_t y = 0;
        int16_t z = 0;
    };

    struct DistantLightsPath
    {
        int16_t centerX = 0;
        int16_t centerY = 0;
        uint16_t sizeX = 0;
        uint16_t sizeY = 0;
        uint16_t nodeIndex = 0;
        uint16_t nodeCount = 0;
        uint16_t short7 = 0;   // HD only
        uint16_t short8 = 0;   // HD only
        float float1 = 0;      // HD only
        uint8_t byte1 = 0;
        uint8_t byte2 = 0;
        uint8_t byte3 = 0;     // HD only
        uint8_t byte4 = 0;     // HD only
    };

    class DistantLightsFile
    {
    public:
        // Parses a distant-lights buffer. `hd` controls grid/cell sizing (32/1024
        // for HD, 16/256 otherwise). Returns false if the buffer is truncated.
        bool load(const std::vector<uint8_t>& data, bool hd = true);

        bool isLoaded() const { return loaded_; }
        bool hd() const { return hd_; }
        uint32_t gridSize() const { return gridSize_; }
        uint32_t cellCount() const { return cellCount_; }
        uint32_t nodeCount() const { return nodeCount_; }
        uint32_t pathCount() const { return pathCount_; }

        const std::vector<DistantLightsNode>& nodes() const { return nodes_; }
        const std::vector<DistantLightsPath>& paths() const { return paths_; }
        const std::vector<uint32_t>& pathIndices() const { return pathIndices_; }

    private:
        bool loaded_ = false;
        bool hd_ = true;
        uint32_t gridSize_ = 32;
        uint32_t cellSize_ = 512;
        uint32_t cellCount_ = 1024;
        uint32_t nodeCount_ = 0;
        uint32_t pathCount_ = 0;
        std::vector<uint32_t> pathIndices_;
        std::vector<uint32_t> pathCounts1_;
        std::vector<uint32_t> pathCounts2_;
        std::vector<DistantLightsNode> nodes_;
        std::vector<DistantLightsPath> paths_;
    };
}
