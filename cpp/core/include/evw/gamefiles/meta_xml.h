// Serializes a parsed Meta (RSC structured metadata) to an XML string, mirroring
// the shape of CodeWalker's MetaXml output. This is a substantial but partial
// port: it handles scalars, float vectors, hashes, enums, char arrays/pointers,
// inline structures, structure pointers and arrays of structures. Unhandled
// field types are emitted as XML comments rather than failing.
#pragma once

#include <string>

#include "evw/gamefiles/meta.h"

namespace evw::gamefiles
{
    // Converts a Meta to indented XML. The root tag is the root block's structure
    // name (resolved via JenkIndex, else "hash_<decimal>").
    std::string metaToXml(const Meta& meta);

    // Resolves a hash to a readable name (JenkIndex string, else "hash_<decimal>").
    std::string metaHashName(uint32_t hash);
}
