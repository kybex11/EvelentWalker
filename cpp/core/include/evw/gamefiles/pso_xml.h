// Serializes a parsed PSO (PSIN metadata) to an XML string using its PSCH
// schema. Partial port: scalars, float vectors, enums and flags are emitted;
// nested structures/arrays/maps and string-table strings are summarized.
#pragma once

#include <string>

#include "evw/gamefiles/pso.h"

namespace evw::gamefiles
{
    // Converts a loaded PSO to indented XML, rooted at the root block's structure.
    std::string psoToXml(const PsoFile& pso);
}
