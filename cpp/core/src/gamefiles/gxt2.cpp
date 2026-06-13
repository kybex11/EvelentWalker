#include "evw/gamefiles/gxt2.h"

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    bool Gxt2File::load(const std::vector<uint8_t>& data)
    {
        entries_.clear();
        if (data.size() < 8) return false;

        DataReader r(data);
        if (r.ReadUInt32() != MAGIC) return false;

        uint32_t count = r.ReadUInt32();
        entries_.resize(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            entries_[i].hash = r.ReadUInt32();
            entries_[i].offset = r.ReadUInt32();
        }

        if (r.ReadUInt32() != MAGIC) return true; // header parsed; strings unavailable
        uint32_t endpos = r.ReadUInt32();

        for (uint32_t i = 0; i < count; ++i)
        {
            auto& e = entries_[i];
            r.setPosition(e.offset);
            std::string s;
            while (r.position() < endpos)
            {
                uint8_t b = r.ReadByte();
                if (b == 0) break;
                s.push_back(static_cast<char>(b));
            }
            e.text = std::move(s);
        }
        return true;
    }

    std::string Gxt2File::lookup(uint32_t hash) const
    {
        for (const auto& e : entries_)
            if (e.hash == hash) return e.text;
        return {};
    }
}
