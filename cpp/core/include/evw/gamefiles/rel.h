// Port of RelFile.cs header — the audio data (.rel) container. This port parses
// the top-level structure: type, data block, name table, index (hash or string),
// hash table and pack table. The per-type RelData record parsing is not ported.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace evw::gamefiles
{
    enum class RelDatFileType : uint32_t
    {
        Dat4 = 4,
        Dat10ModularSynth = 10,
        Dat15DynamicMixer = 15,
        Dat16Curves = 16,
        Dat22Categories = 22,
        Dat54DataEntries = 54,
        Dat149 = 149,
        Dat150 = 150,
        Dat151 = 151,
    };

    // An index entry referenced by hash (most .rel files).
    struct RelIndexHash
    {
        uint32_t name = 0;   // MetaHash
        uint32_t offset = 0;
        uint32_t length = 0;
    };

    // An index entry referenced by string name (audioconfig dat4).
    struct RelIndexString
    {
        std::string name;
        uint32_t offset = 0;
        uint32_t length = 0;
    };

    class RelFile
    {
    public:
        // Parses a .rel buffer. Returns false if the buffer is too small.
        bool load(const std::vector<uint8_t>& data);

        bool isLoaded() const { return loaded_; }

        RelDatFileType relType() const { return relType_; }
        uint32_t dataLength() const { return dataLength_; }
        const std::vector<uint8_t>& dataBlock() const { return dataBlock_; }
        bool isAudioConfig() const { return isAudioConfig_; }

        const std::vector<std::string>& nameTable() const { return nameTable_; }
        const std::vector<RelIndexHash>& indexHashes() const { return indexHashes_; }
        const std::vector<RelIndexString>& indexStrings() const { return indexStrings_; }
        const std::vector<uint32_t>& hashTable() const { return hashTable_; }
        const std::vector<uint32_t>& packTable() const { return packTable_; }

    private:
        bool loaded_ = false;
        RelDatFileType relType_ = RelDatFileType::Dat4;
        uint32_t dataLength_ = 0;
        std::vector<uint8_t> dataBlock_;
        uint32_t nameTableLength_ = 0;
        bool isAudioConfig_ = false;
        std::vector<std::string> nameTable_;
        std::vector<RelIndexHash> indexHashes_;
        std::vector<RelIndexString> indexStrings_;
        std::vector<uint32_t> hashTable_;
        std::vector<uint32_t> packTable_;
    };
}
