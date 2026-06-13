// Port of the DXT (BC1/BC2/BC3) decompression from DDSIO.cs.
// Decodes block-compressed texture data into 32-bit RGBA pixels, enabling
// previews/conversion without external tools.
#pragma once

#include <cstdint>
#include <vector>

#include "evw/gamefiles/texture.h"

namespace evw::texconv
{
    // Decode raw block data to width*height*4 RGBA bytes.
    std::vector<uint8_t> decompressDxt1(const std::vector<uint8_t>& data, int width, int height);
    std::vector<uint8_t> decompressDxt3(const std::vector<uint8_t>& data, int width, int height);
    std::vector<uint8_t> decompressDxt5(const std::vector<uint8_t>& data, int width, int height);

    // Decodes the top mip of a texture to RGBA8 for supported formats
    // (DXT1/DXT3/DXT5, and 32-bit uncompressed). Returns empty if unsupported.
    std::vector<uint8_t> decodeToRGBA(const gamefiles::Texture& texture);
}
