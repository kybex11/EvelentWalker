#include "evw/gamefiles/heightmap.h"

#include <algorithm>

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    bool HeightmapFile::load(const std::vector<uint8_t>& data)
    {
        if (data.size() < 36) return false;
        DataReader r(data);

        magic = r.ReadUInt32();
        r.ReadByte();                   // VersionMajor
        r.ReadByte();                   // VersionMinor
        r.ReadUInt16();                 // Pad
        compressed = r.ReadUInt32();
        width = r.ReadUInt16();
        height = r.ReadUInt16();
        bbMin = r.ReadVector3();
        bbMax = r.ReadVector3();
        uint32_t length = r.ReadUInt32();

        int dlen = static_cast<int>(length);
        std::vector<HeightmapCompHeader> headers;
        if (compressed > 0)
        {
            headers.resize(height);
            for (int i = 0; i < height; ++i)
            {
                headers[i].start = r.ReadUInt16();
                headers[i].count = r.ReadUInt16();
                headers[i].dataOffset = r.ReadUInt32();
            }
            dlen -= height * 8;
        }
        if (dlen < 0) return false;

        std::vector<uint8_t> d = r.ReadBytes(dlen);

        maxHeights.assign(static_cast<size_t>(width) * height, 0);
        minHeights.assign(static_cast<size_t>(width) * height, 0);

        if (compressed > 0)
        {
            int h2off = dlen / 2;
            for (int y = 0; y < height; ++y)
            {
                const auto& h = headers[y];
                for (int i = 0; i < h.count; ++i)
                {
                    int x = h.start + i;
                    int o = static_cast<int>(h.dataOffset) + x;
                    if (x < width && o >= 0 && o + h2off < static_cast<int>(d.size()))
                    {
                        maxHeights[y * width + x] = d[o];
                        minHeights[y * width + x] = d[o + h2off];
                    }
                }
            }
        }
        else
        {
            size_t n = std::min(d.size(), maxHeights.size());
            for (size_t i = 0; i < n; ++i) { maxHeights[i] = d[i]; minHeights[i] = d[i]; }
        }
        return true;
    }
}
