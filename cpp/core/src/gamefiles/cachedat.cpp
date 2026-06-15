#include "evw/gamefiles/cachedat.h"

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    namespace
    {
        std::string stripVersion(std::string s)
        {
            // Remove the "[VERSION]" marker and any CR/LF characters.
            auto pos = s.find("[VERSION]");
            if (pos != std::string::npos) s.erase(pos, 9);
            std::string out;
            for (char c : s) if (c != '\r' && c != '\n') out.push_back(c);
            return out;
        }
    }

    bool CacheDatFile::load(const std::vector<uint8_t>& data)
    {
        loaded_ = false;
        version_.clear();
        fileDates_.clear();
        boundsStore_.clear();
        mapDataStoreLength_ = 0;
        interiorProxyCount_ = 0;
        if (data.empty()) return false;

        // Version string lives in the first 100 bytes, null-terminated.
        std::string v;
        for (size_t i = 0; i < 100 && i < data.size(); ++i)
        {
            if (data[i] == 0) break;
            v.push_back(static_cast<char>(data[i]));
        }
        version_ = stripVersion(v);

        DataReader r(data, Endianess::LittleEndian);
        bool inDates = false;
        std::string line;

        for (size_t i = 100; i < data.size(); ++i)
        {
            uint8_t b = data[i];
            if (b == 0) break;
            if (b == 0x0A) // newline -> end of a tag/line
            {
                if (line == "<fileDates>") inDates = true;
                else if (line == "</fileDates>") inDates = false;
                else if (line == "fwMapDataStore")
                {
                    r.setPosition(static_cast<int64_t>(i) + 1);
                    uint32_t modlen = r.ReadUInt32();
                    mapDataStoreLength_ = modlen;
                    i += static_cast<size_t>(modlen) + 4;
                }
                else if (line == "CInteriorProxy")
                {
                    r.setPosition(static_cast<int64_t>(i) + 1);
                    uint32_t modlen = r.ReadUInt32();
                    interiorProxyCount_ = modlen / 104;
                    i += static_cast<size_t>(modlen) + 4;
                }
                else if (line == "BoundsStore")
                {
                    r.setPosition(static_cast<int64_t>(i) + 1);
                    uint32_t modlen = r.ReadUInt32();
                    int64_t end = static_cast<int64_t>(i) + modlen + 5;
                    while (r.position() < end && r.position() + 32 <= r.length())
                    {
                        BoundsStoreItem item;
                        item.name = r.ReadUInt32();
                        item.min = r.ReadVector3();
                        item.max = r.ReadVector3();
                        item.layer = r.ReadUInt32();
                        boundsStore_.push_back(item);
                    }
                    i += static_cast<size_t>(modlen) + 4;
                }
                else if (inDates && !line.empty())
                {
                    fileDates_.push_back(line);
                }
                line.clear();
            }
            else
            {
                line.push_back(static_cast<char>(b));
            }
        }

        loaded_ = true;
        return true;
    }
}
