// Minimal, self-contained SHA-1 (FIPS 180-1). Used by the GTA5 key search,
// which locates keys inside gta5.exe by matching the SHA-1 of candidate blocks.
#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace evw::crypto
{
    // Computes the 20-byte SHA-1 digest of `data`.
    std::array<uint8_t, 20> sha1(const uint8_t* data, size_t length);
    std::array<uint8_t, 20> sha1(const std::vector<uint8_t>& data);
}
