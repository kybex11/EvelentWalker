#include "evw/gamefiles/distantlights.h"

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    bool DistantLightsFile::load(const std::vector<uint8_t>& data, bool hd)
    {
        loaded_ = false;
        hd_ = hd;
        if (hd)
        {
            gridSize_ = 32; cellSize_ = 512; cellCount_ = 1024;
        }
        else
        {
            gridSize_ = 16; cellSize_ = 1024; cellCount_ = 256;
        }

        DataReader r(data, Endianess::BigEndian);
        if (r.length() < 8) return false;

        nodeCount_ = r.ReadUInt32();
        pathCount_ = r.ReadUInt32();

        pathIndices_.resize(cellCount_);
        pathCounts1_.resize(cellCount_);
        pathCounts2_.resize(cellCount_);
        for (uint32_t i = 0; i < cellCount_; ++i) pathIndices_[i] = r.ReadUInt32();
        for (uint32_t i = 0; i < cellCount_; ++i) pathCounts1_[i] = r.ReadUInt32();
        for (uint32_t i = 0; i < cellCount_; ++i) pathCounts2_[i] = r.ReadUInt32();

        nodes_.resize(nodeCount_);
        for (uint32_t i = 0; i < nodeCount_; ++i)
        {
            DistantLightsNode n;
            n.x = r.ReadInt16();
            n.y = r.ReadInt16();
            n.z = r.ReadInt16();
            nodes_[i] = n;
        }

        paths_.resize(pathCount_);
        for (uint32_t i = 0; i < pathCount_; ++i)
        {
            DistantLightsPath p;
            p.centerX = r.ReadInt16();
            p.centerY = r.ReadInt16();
            p.sizeX = r.ReadUInt16();
            p.sizeY = r.ReadUInt16();
            p.nodeIndex = r.ReadUInt16();
            p.nodeCount = r.ReadUInt16();
            if (hd_)
            {
                p.short7 = r.ReadUInt16();
                p.short8 = r.ReadUInt16();
                p.float1 = r.ReadSingle();
                p.byte1 = r.ReadByte();
                p.byte2 = r.ReadByte();
                p.byte3 = r.ReadByte();
                p.byte4 = r.ReadByte();
            }
            else
            {
                p.byte1 = r.ReadByte();
                p.byte2 = r.ReadByte();
            }
            paths_[i] = p;
        }

        loaded_ = true;
        return true;
    }
}
