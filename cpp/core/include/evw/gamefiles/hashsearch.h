// Port of HashSearch (GTAKeys.cs) — locates fixed-size key blocks inside a
// binary (e.g. gta5.exe) by matching the SHA-1 of candidate windows at 8-byte
// aligned offsets. Mirrors CodeWalker's parallel block scan.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace evw::gamefiles
{
    // For each target SHA-1 in `hashes`, returns the `length`-byte block whose
    // SHA-1 matches (empty vector if not found). Windows are tested at every
    // 8-byte aligned offset, matching the original implementation.
    std::vector<std::vector<uint8_t>> hashSearch(const std::vector<uint8_t>& data,
                                                 const std::vector<std::array<uint8_t, 20>>& hashes,
                                                 int length);

    // Convenience: search for a single hash; returns empty if not found.
    std::vector<uint8_t> hashSearchOne(const std::vector<uint8_t>& data,
                                       const std::array<uint8_t, 20>& hash, int length);
}
