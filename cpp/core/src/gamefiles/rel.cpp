#include "evw/gamefiles/rel.h"

#include "evw/gamefiles/data.h"
#include "evw/gamefiles/jenk.h"

#include <cctype>

namespace evw::gamefiles
{
    namespace
    {
        std::string toLower(std::string s)
        {
            for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }
    }

    bool RelFile::load(const std::vector<uint8_t>& data)
    {
        loaded_ = false;
        if (data.size() < 8) return false;

        DataReader r(data, Endianess::LittleEndian);

        relType_ = static_cast<RelDatFileType>(r.ReadUInt32());
        dataLength_ = r.ReadUInt32();
        if (static_cast<int64_t>(r.position()) + dataLength_ > r.length()) return false;
        dataBlock_ = r.ReadBytes(static_cast<int>(dataLength_));

        nameTableLength_ = r.ReadUInt32();
        uint32_t nameTableCount = r.ReadUInt32();
        nameTable_.clear();
        if (nameTableCount > 0)
        {
            // Skip the offset table (one uint32 per name).
            r.setPosition(r.position() + static_cast<int64_t>(nameTableCount) * 4);
            nameTable_.reserve(nameTableCount);
            for (uint32_t i = 0; i < nameTableCount; ++i)
            {
                std::string s;
                while (true)
                {
                    char c = static_cast<char>(r.ReadByte());
                    if (c == 0) break;
                    s.push_back(c);
                }
                JenkIndex::Ensure(toLower(s));
                nameTable_.push_back(std::move(s));
            }
        }

        uint32_t indexCount = r.ReadUInt32();
        indexHashes_.clear();
        indexStrings_.clear();
        if (indexCount > 0)
        {
            if (relType_ == RelDatFileType::Dat4 && nameTableLength_ == 4)
            {
                isAudioConfig_ = true;
                r.ReadUInt32(); // IndexStringFlags (usually 2524)
                indexStrings_.reserve(indexCount);
                for (uint32_t i = 0; i < indexCount; ++i)
                {
                    uint8_t sl = r.ReadByte();
                    RelIndexString ris;
                    for (int j = 0; j < sl; ++j)
                    {
                        char c = static_cast<char>(r.ReadByte());
                        if (c != 0) ris.name.push_back(c);
                    }
                    ris.offset = r.ReadUInt32();
                    ris.length = r.ReadUInt32();
                    JenkIndex::Ensure(toLower(ris.name));
                    indexStrings_.push_back(std::move(ris));
                }
            }
            else
            {
                indexHashes_.reserve(indexCount);
                for (uint32_t i = 0; i < indexCount; ++i)
                {
                    RelIndexHash rih;
                    rih.name = r.ReadUInt32();
                    rih.offset = r.ReadUInt32();
                    rih.length = r.ReadUInt32();
                    indexHashes_.push_back(rih);
                }
            }
        }

        uint32_t hashTableCount = r.ReadUInt32();
        hashTable_.clear();
        if (hashTableCount != 0)
        {
            hashTable_.reserve(hashTableCount);
            for (uint32_t i = 0; i < hashTableCount; ++i)
            {
                uint32_t off = r.ReadUInt32();
                int64_t pos = r.position();
                r.setPosition(off);
                hashTable_.push_back(r.ReadUInt32());
                r.setPosition(pos);
            }
        }

        uint32_t packTableCount = r.ReadUInt32();
        packTable_.clear();
        if (packTableCount != 0)
        {
            packTable_.reserve(packTableCount);
            for (uint32_t i = 0; i < packTableCount; ++i)
            {
                uint32_t off = r.ReadUInt32();
                int64_t pos = r.position();
                r.setPosition(off);
                packTable_.push_back(r.ReadUInt32());
                r.setPosition(pos);
            }
        }

        loaded_ = true;
        return true;
    }
}
