#include "evw/gamefiles/pso.h"

#include "evw/gamefiles/data.h"

#include <cstring>

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
        schema_.clear();
        enums_.clear();
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
            case PsoSection::PSCH:
            {
                sr.ReadInt32();            // Ident
                sr.ReadInt32();            // Length
                uint32_t count = sr.ReadUInt32();
                struct IdxInfo { uint32_t nameHash; int32_t offset; };
                std::vector<IdxInfo> idx(count);
                for (uint32_t i = 0; i < count; ++i)
                {
                    idx[i].nameHash = sr.ReadUInt32();
                    idx[i].offset = sr.ReadInt32();
                }
                for (uint32_t i = 0; i < count; ++i)
                {
                    sr.setPosition(idx[i].offset);
                    uint8_t type = sr.ReadByte();
                    sr.setPosition(idx[i].offset);
                    if (type == 0) // structure
                    {
                        uint32_t x = sr.ReadUInt32();
                        int16_t entriesCount = static_cast<int16_t>(x & 0xFFFF);
                        PsoSchemaStructure s;
                        s.nameHash = idx[i].nameHash;
                        s.structureLength = sr.ReadInt32();
                        sr.ReadUInt32();   // Unk_Ch
                        s.entries.resize(entriesCount > 0 ? entriesCount : 0);
                        for (int j = 0; j < entriesCount; ++j)
                        {
                            PsoSchemaEntry e;
                            e.entryNameHash = sr.ReadUInt32();
                            e.type = sr.ReadByte();
                            e.unk5 = sr.ReadByte();
                            e.dataOffset = sr.ReadUInt16();
                            e.referenceKey = sr.ReadUInt32();
                            s.entries[j] = e;
                        }
                        schema_.push_back(std::move(s));
                    }
                    else if (type == 1) // enum
                    {
                        uint32_t x = sr.ReadUInt32();
                        int32_t entriesCount = static_cast<int32_t>(x & 0x00FFFFFF);
                        PsoSchemaEnum en;
                        en.nameHash = idx[i].nameHash;
                        en.entries.resize(entriesCount > 0 ? entriesCount : 0);
                        for (int j = 0; j < entriesCount; ++j)
                        {
                            PsoSchemaEnumEntry e;
                            e.entryNameHash = sr.ReadUInt32();
                            e.entryKey = sr.ReadInt32();
                            en.entries[j] = e;
                        }
                        enums_.push_back(std::move(en));
                    }
                }
                break;
            }
            default:
                break; // STR*/PSIG/CHKS not parsed yet
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

    const PsoSchemaStructure* PsoFile::findSchema(uint32_t nameHash) const
    {
        for (const auto& s : schema_)
            if (s.nameHash == nameHash) return &s;
        return nullptr;
    }

    const PsoSchemaEnum* PsoFile::findEnum(uint32_t nameHash) const
    {
        for (const auto& e : enums_)
            if (e.nameHash == nameHash) return &e;
        return nullptr;
    }

    uint8_t PsoFile::readByteAt(int32_t offset) const
    {
        if (offset < 0 || offset >= static_cast<int32_t>(dataSection_.size())) return 0;
        return dataSection_[offset];
    }

    uint16_t PsoFile::readUInt16At(int32_t offset) const
    {
        if (offset < 0 || offset + 2 > static_cast<int32_t>(dataSection_.size())) return 0;
        return (static_cast<uint16_t>(dataSection_[offset]) << 8) |
               static_cast<uint16_t>(dataSection_[offset + 1]);
    }

    uint32_t PsoFile::readUInt32At(int32_t offset) const
    {
        if (offset < 0 || offset + 4 > static_cast<int32_t>(dataSection_.size())) return 0;
        return (static_cast<uint32_t>(dataSection_[offset]) << 24) |
               (static_cast<uint32_t>(dataSection_[offset + 1]) << 16) |
               (static_cast<uint32_t>(dataSection_[offset + 2]) << 8) |
               static_cast<uint32_t>(dataSection_[offset + 3]);
    }

    int32_t PsoFile::readInt32At(int32_t offset) const
    {
        return static_cast<int32_t>(readUInt32At(offset));
    }

    float PsoFile::readFloatAt(int32_t offset) const
    {
        uint32_t bits = readUInt32At(offset);
        float f;
        std::memcpy(&f, &bits, sizeof(f));
        return f;
    }

    int32_t PsoFile::fieldOffset(int id, uint16_t field) const
    {
        const PsoDataMappingEntry* block = getBlock(id);
        if (!block) return -1;
        // The data section buffer includes the 8-byte PSIN section header, so a
        // block's payload begins at (8 + block->offset).
        return 8 + block->offset + static_cast<int32_t>(field);
    }

    uint32_t PsoFile::readBlockUInt32(int id, uint16_t field) const
    {
        int32_t off = fieldOffset(id, field);
        return off < 0 ? 0u : readUInt32At(off);
    }

    int32_t PsoFile::readBlockInt32(int id, uint16_t field) const
    {
        int32_t off = fieldOffset(id, field);
        return off < 0 ? 0 : readInt32At(off);
    }

    float PsoFile::readBlockFloat(int id, uint16_t field) const
    {
        int32_t off = fieldOffset(id, field);
        return off < 0 ? 0.0f : readFloatAt(off);
    }
}
