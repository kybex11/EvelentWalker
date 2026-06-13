// Port of the DDS-writing part of EvelentWalker.Core/GameFiles/Utils/DDSIO.cs.
// Produces a standard .dds file (magic + 124-byte DDS_HEADER [+ DX10 ext for BC7])
// followed by the texture's raw mip-chained pixel data, for the texture formats
// used by GTA V. The full DirectXTex decode pipeline is not ported.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "evw/gamefiles/texture.h"

namespace evw::texconv
{
    // Builds a complete .dds file from a parsed texture. Throws std::runtime_error
    // if the texture has no pixel data.
    std::vector<uint8_t> getDDSFile(const gamefiles::Texture& texture);

    // Convenience: writes getDDSFile(texture) to a file path. Returns false on I/O error.
    bool writeDDSToFile(const gamefiles::Texture& texture, const std::string& path);
}
