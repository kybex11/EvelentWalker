// Self-contained raw DEFLATE (RFC 1951) decompressor.
// Replaces System.IO.Compression.DeflateStream (decompress path) used for
// magic.dat key data and RPF resource decompression.
#pragma once

#include <cstdint>
#include <vector>

namespace evw::compression
{
    // Inflates a raw DEFLATE stream (no zlib/gzip header).
    // If expectedSize > 0 the output buffer is reserved accordingly.
    // Throws std::runtime_error on malformed input.
    std::vector<uint8_t> inflateRaw(const std::vector<uint8_t>& input, size_t expectedSize = 0);
    std::vector<uint8_t> inflateRaw(const uint8_t* data, size_t length, size_t expectedSize = 0);
}
