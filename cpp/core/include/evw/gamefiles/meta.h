// Port of EvelentWalker.Core/GameFiles/MetaTypes/Meta.cs (read path).
// Meta is the RSC structured-metadata container used by Ymap/Ytyp/Ymt and many
// other files: a set of structure definitions, enum definitions and raw data
// blocks, with a designated root block.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "evw/gamefiles/resource_data.h"

namespace evw::gamefiles
{
    enum class MetaStructureEntryDataType : uint8_t
    {
        Boolean = 0x01,
        SignedByte = 0x10,
        UnsignedByte = 0x11,
        SignedShort = 0x12,
        UnsignedShort = 0x13,
        SignedInt = 0x14,
        UnsignedInt = 0x15,
        Float = 0x21,
        Float_XYZ = 0x33,
        Float_XYZW = 0x34,
        ByteEnum = 0x60,
        IntEnum = 0x62,
        ShortFlags = 0x64,
        IntFlags1 = 0x63,
        IntFlags2 = 0x65,
        Hash = 0x4A,
        Array = 0x52,
        ArrayOfChars = 0x40,
        ArrayOfBytes = 0x50,
        DataBlockPointer = 0x59,
        CharPointer = 0x44,
        StructurePointer = 0x07,
        Structure = 0x05,
    };

#pragma pack(push, 1)
    struct MetaStructureEntryInfo_s
    {
        uint32_t entryNameHash;
        int32_t dataOffset;
        uint8_t dataType;          // MetaStructureEntryDataType
        uint8_t unknown9;
        int16_t referenceTypeIndex;
        uint32_t referenceKey;
    };
    static_assert(sizeof(MetaStructureEntryInfo_s) == 16, "MetaStructureEntryInfo_s must be 16 bytes");

    struct MetaEnumEntryInfo_s
    {
        uint32_t entryNameHash;
        int32_t entryValue;
    };
    static_assert(sizeof(MetaEnumEntryInfo_s) == 8, "MetaEnumEntryInfo_s must be 8 bytes");
#pragma pack(pop)

    struct MetaStructureInfo
    {
        uint32_t structureNameHash = 0;
        uint32_t structureKey = 0;
        int64_t entriesPointer = 0;
        int32_t structureSize = 0;
        int16_t entriesCount = 0;
        std::vector<MetaStructureEntryInfo_s> entries;

        void read(ResourceDataReader& r);
    };

    struct MetaEnumInfo
    {
        uint32_t enumNameHash = 0;
        uint32_t enumKey = 0;
        int64_t entriesPointer = 0;
        int32_t entriesCount = 0;
        std::vector<MetaEnumEntryInfo_s> entries;

        void read(ResourceDataReader& r);
    };

    struct MetaDataBlock
    {
        uint32_t structureNameHash = 0;
        int32_t dataLength = 0;
        int64_t dataPointer = 0;
        std::vector<uint8_t> data;

        void read(ResourceDataReader& r);
    };

    struct Meta : public ResourceFileBase
    {
        uint8_t hasEncryptedStrings = 0;
        int32_t rootBlockIndex = 0;
        int64_t structureInfosPointer = 0;
        int64_t enumInfosPointer = 0;
        int64_t dataBlocksPointer = 0;
        int64_t namePointer = 0;
        int16_t structureInfosCount = 0;
        int16_t enumInfosCount = 0;
        int16_t dataBlocksCount = 0;

        ResourceSimpleArray<MetaStructureInfo> structureInfos;
        ResourceSimpleArray<MetaEnumInfo> enumInfos;
        ResourceSimpleArray<MetaDataBlock> dataBlocks;
        std::string name;

        void read(ResourceDataReader& r);

        // Finds a structure definition by its name hash (or nullptr).
        const MetaStructureInfo* findStructure(uint32_t nameHash) const;
        // Returns the root data block, or nullptr.
        const MetaDataBlock* rootBlock() const;
    };
}
