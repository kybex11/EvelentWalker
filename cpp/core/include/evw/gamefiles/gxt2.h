// Port of Gxt2File.cs — the GXT2 global text table (hash -> UTF-8 string).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace evw::gamefiles
{
    struct Gxt2Entry
    {
        uint32_t hash = 0;
        uint32_t offset = 0;
        std::string text;
    };

    class Gxt2File
    {
    public:
        // Parses a GXT2 buffer. Returns false if the magic does not match.
        bool load(const std::vector<uint8_t>& data);

        const std::vector<Gxt2Entry>& entries() const { return entries_; }

        // Looks up the text for a hash, or empty string.
        std::string lookup(uint32_t hash) const;

        static constexpr uint32_t MAGIC = 1196971058u; // "GXT2"

    private:
        std::vector<Gxt2Entry> entries_;
    };
}
