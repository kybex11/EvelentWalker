#include "evw/gamefiles/fxc.h"

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    namespace
    {
        // FxcFile.ReadString: 1-byte length prefix (incl. null terminator),
        // then (len-1) ASCII chars. Length 0 -> empty string.
        std::string readFxcString(DataReader& r)
        {
            uint8_t sl = r.ReadByte();
            if (sl == 0) return {};
            auto bytes = r.ReadBytes(sl);
            // Last byte is a null terminator.
            size_t n = sl > 1 ? static_cast<size_t>(sl - 1) : 0;
            return std::string(reinterpret_cast<const char*>(bytes.data()), n);
        }
    }

    bool FxcFile::load(const std::vector<uint8_t>& data)
    {
        loaded_ = false;
        presetParams_.clear();
        if (data.size() < 9) return false;

        DataReader r(data, Endianess::LittleEndian);
        if (r.ReadUInt32() != MAGIC) return false;
        vertexType_ = r.ReadUInt32();

        uint8_t ppCount = r.ReadByte();
        presetParams_.reserve(ppCount);
        for (uint8_t i = 0; i < ppCount; ++i)
        {
            FxcPresetParam p;
            p.name = readFxcString(r);
            r.ReadByte();           // Unused0 (always 0)
            p.value = r.ReadUInt32();
            presetParams_.push_back(std::move(p));
        }

        loaded_ = true;
        return true;
    }
}
