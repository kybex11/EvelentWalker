// Port of the core of Pso.cs — the PSO ("PSIN") metadata container used by many
// .ymt/.meta files. It's a big-endian, section-based format. This port parses
// the data section (PSIN), the data-map section (PMAP) and exposes block access;
// the full schema (PSCH) parsing is not ported yet.
#pragma once

#include <cstdint>
#include <vector>

namespace evw::gamefiles
{
    enum class PsoSection : uint32_t
    {
        PSIN = 0x5053494E,
        PMAP = 0x504D4150,
        PSCH = 0x50534348,
        STRF = 0x53545246,
        STRS = 0x53545253,
        STRE = 0x53545245,
        PSIG = 0x50534947,
        CHKS = 0x43484B53,
    };

    struct PsoDataMappingEntry
    {
        uint32_t nameHash = 0;
        int32_t offset = 0;
        int32_t unknown8 = 0;
        int32_t length = 0;
    };

    // A field within a PSO schema structure.
    struct PsoSchemaEntry
    {
        uint32_t entryNameHash = 0;
        uint8_t type = 0;          // PsoDataType
        uint8_t unk5 = 0;
        uint16_t dataOffset = 0;
        uint32_t referenceKey = 0;
    };

    // A structure definition in the PSO schema (PSCH).
    struct PsoSchemaStructure
    {
        uint32_t nameHash = 0;
        int32_t structureLength = 0;
        std::vector<PsoSchemaEntry> entries;
    };

    // An entry within a PSO enum definition.
    struct PsoSchemaEnumEntry
    {
        uint32_t entryNameHash = 0;
        int32_t entryKey = 0;
    };

    // An enum definition in the PSO schema (PSCH).
    struct PsoSchemaEnum
    {
        uint32_t nameHash = 0;
        std::vector<PsoSchemaEnumEntry> entries;
    };

    class PsoFile
    {
    public:
        // Parses a PSO file from raw bytes. Returns false if not a PSO.
        bool load(const std::vector<uint8_t>& data);

        bool isLoaded() const { return loaded_; }
        int rootId() const { return rootId_; }

        // The raw PSIN data section (includes its 8-byte section header).
        const std::vector<uint8_t>& dataSection() const { return dataSection_; }
        const std::vector<PsoDataMappingEntry>& entries() const { return entries_; }
        const std::vector<PsoSchemaStructure>& schema() const { return schema_; }
        const std::vector<PsoSchemaEnum>& enums() const { return enums_; }

        // Returns the data-map entry for a 1-based block id, or nullptr.
        const PsoDataMappingEntry* getBlock(int id) const;

        // Finds a schema structure by its name hash, or nullptr.
        const PsoSchemaStructure* findSchema(uint32_t nameHash) const;

        // Finds a schema enum by its name hash, or nullptr.
        const PsoSchemaEnum* findEnum(uint32_t nameHash) const;

        // ---- Typed value access into the data (PSIN) section ----
        // All reads are big-endian, matching the on-disk PSO layout. `offset` is
        // an absolute byte offset into the data section (which includes the
        // 8-byte section header). Out-of-range reads return 0.
        uint8_t  readByteAt(int32_t offset) const;
        uint16_t readUInt16At(int32_t offset) const;
        uint32_t readUInt32At(int32_t offset) const;
        int32_t  readInt32At(int32_t offset) const;
        float    readFloatAt(int32_t offset) const;

        // Computes the absolute data-section offset of a field within a block.
        // Block `id` is 1-based; `fieldOffset` is the field's dataOffset from the
        // schema. The 8-byte PSIN section header is accounted for. Returns -1 if
        // the block id is invalid.
        int32_t fieldOffset(int id, uint16_t fieldOffset) const;

        // Reads a typed field of a block by 1-based id and schema field offset.
        uint32_t readBlockUInt32(int id, uint16_t fieldOffset) const;
        int32_t  readBlockInt32(int id, uint16_t fieldOffset) const;
        float    readBlockFloat(int id, uint16_t fieldOffset) const;

        // Detects whether the buffer is a PSO (first 4 bytes == "PSIN").
        static bool isPSO(const std::vector<uint8_t>& data);

    private:
        bool loaded_ = false;
        int rootId_ = 0;
        std::vector<uint8_t> dataSection_;
        std::vector<PsoDataMappingEntry> entries_;
        std::vector<PsoSchemaStructure> schema_;
        std::vector<PsoSchemaEnum> enums_;
    };
}
