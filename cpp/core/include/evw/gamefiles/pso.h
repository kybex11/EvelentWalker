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

        // Returns the data-map entry for a 1-based block id, or nullptr.
        const PsoDataMappingEntry* getBlock(int id) const;

        // Detects whether the buffer is a PSO (first 4 bytes == "PSIN").
        static bool isPSO(const std::vector<uint8_t>& data);

    private:
        bool loaded_ = false;
        int rootId_ = 0;
        std::vector<uint8_t> dataSection_;
        std::vector<PsoDataMappingEntry> entries_;
    };
}
