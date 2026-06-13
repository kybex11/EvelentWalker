#include "evw/gamefiles/meta.h"

namespace evw::gamefiles
{
    void MetaStructureInfo::read(ResourceDataReader& r)
    {
        structureNameHash = r.ReadUInt32();
        structureKey = r.ReadUInt32();
        r.ReadUInt32();                 // Unknown_8h
        r.ReadUInt32();                 // Unknown_Ch
        entriesPointer = r.ReadInt64();
        structureSize = r.ReadInt32();
        r.ReadInt16();                  // Unknown_1Ch
        entriesCount = r.ReadInt16();

        entries = r.ReadStructsAt<MetaStructureEntryInfo_s>(
            static_cast<uint64_t>(entriesPointer), static_cast<uint32_t>(entriesCount));
    }

    void MetaEnumInfo::read(ResourceDataReader& r)
    {
        enumNameHash = r.ReadUInt32();
        enumKey = r.ReadUInt32();
        entriesPointer = r.ReadInt64();
        entriesCount = r.ReadInt32();
        r.ReadInt32();                  // Unknown_14h

        entries = r.ReadStructsAt<MetaEnumEntryInfo_s>(
            static_cast<uint64_t>(entriesPointer), static_cast<uint32_t>(entriesCount));
    }

    void MetaDataBlock::read(ResourceDataReader& r)
    {
        structureNameHash = r.ReadUInt32();
        dataLength = r.ReadInt32();
        dataPointer = r.ReadInt64();
        data = r.ReadBytesAt(static_cast<uint64_t>(dataPointer), static_cast<uint32_t>(dataLength));
    }

    void Meta::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        r.ReadInt32();                  // Unknown_10h
        r.ReadInt16();                  // Unknown_14h
        hasEncryptedStrings = r.ReadByte();
        r.ReadByte();                   // Unknown_17h
        r.ReadInt32();                  // Unknown_18h
        rootBlockIndex = r.ReadInt32();
        structureInfosPointer = r.ReadInt64();
        enumInfosPointer = r.ReadInt64();
        dataBlocksPointer = r.ReadInt64();
        namePointer = r.ReadInt64();
        r.ReadInt64();                  // EncryptedStringsPointer
        structureInfosCount = r.ReadInt16();
        enumInfosCount = r.ReadInt16();
        dataBlocksCount = r.ReadInt16();
        r.ReadInt16();                  // Unknown_4Eh
        for (int i = 0; i < 8; ++i) r.ReadUInt32(); // Unknown_50h..6Ch

        auto readArray = [&](auto& arr, int64_t ptr, int count) {
            if (ptr == 0 || count <= 0) return;
            uint64_t backup = r.position();
            r.setPosition(static_cast<uint64_t>(ptr));
            arr.read(r, static_cast<uint32_t>(count));
            r.setPosition(backup);
        };
        readArray(structureInfos, structureInfosPointer, structureInfosCount);
        readArray(enumInfos, enumInfosPointer, enumInfosCount);
        readArray(dataBlocks, dataBlocksPointer, dataBlocksCount);

        name = r.ReadStringAt(static_cast<uint64_t>(namePointer));
    }

    const MetaStructureInfo* Meta::findStructure(uint32_t nameHash) const
    {
        for (const auto& s : structureInfos.data)
            if (s.structureNameHash == nameHash) return &s;
        return nullptr;
    }

    const MetaDataBlock* Meta::rootBlock() const
    {
        int idx = rootBlockIndex - 1; // RootBlockIndex is 1-based in RSC meta
        if (idx >= 0 && idx < static_cast<int>(dataBlocks.data.size()))
            return &dataBlocks.data[idx];
        return nullptr;
    }
}
