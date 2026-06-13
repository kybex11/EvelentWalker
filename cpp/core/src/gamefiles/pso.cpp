#include "evw/gamefiles/pso.h"

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    bool PsoFile::isPSO(const std::vector<uint8_t>& data)
    {
        if (data.size() < 4) return false;
        // Big-endian "PSIN" == 0x5053494E.
        uint32_t ident = (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
                         (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
        return ident == 0x5053494Eu;
    }

    bool PsoFile::load(const std::vector<uint8_t>& data)
    {
        loaded_ = false;
        entries_.clear();
        dataSection_.clear();
        rootId_ = 0;
        if (!isPSO(data)) return false;

        DataReader reader(data, Endianess::BigEndian);
        const int64_t len = reader.length();

        while (reader.position() + 8 <= len)
        {
            uint32_t identInt = reader.ReadUInt32();
            int32_t sectionLen = reader.ReadInt32();
            reader.setPosition(reader.position() - 8);
            if (sectionLen <= 0 || reader.position() + sectionLen > len) break;

            std::vector<uint8_t> section = reader.ReadBytes(sectionLen);
            DataReader sr(section, Endianess::BigEndian);

            switch (static_cast<PsoSection>(identInt))
            {
            case PsoSection::PSIN:
                dataSection_ = section; // includes the 8-byte section header
                break;
            case PsoSection::PMAP:
            {
                sr.ReadInt32();            // Ident
                sr.ReadInt32();            // Length
                rootId_ = sr.ReadInt32();
                int16_t entriesCount = sr.ReadInt16();
                sr.ReadInt16();            // Unknown_Eh
                if (entriesCount <= 0)
                {
                    entriesCount = sr.ReadInt16();
                    sr.ReadInt16(); sr.ReadInt16(); sr.ReadInt16();
                }
                entries_.resize(entriesCount > 0 ? entriesCount : 0);
                for (int i = 0; i < entriesCount; ++i)
                {
                    PsoDataMappingEntry e;
                    e.nameHash = sr.ReadUInt32();
                    e.offset = sr.ReadInt32();
                    e.unknown8 = sr.ReadInt32();
                    e.length = sr.ReadInt32();
                    entries_[i] = e;
                }
                break;
            }
            default:
                break; // PSCH/STR*/PSIG/CHKS not parsed yet
            }
        }

        loaded_ = true;
        return true;
    }

    const PsoDataMappingEntry* PsoFile::getBlock(int id) const
    {
        int idx = id - 1;
        if (idx >= 0 && idx < static_cast<int>(entries_.size())) return &entries_[idx];
        return nullptr;
    }
}
