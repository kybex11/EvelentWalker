// Port of the NavMesh header from Nav.cs (Ynv). Parses the navmesh header
// (bounds, counts, area id). The paged vertex/poly lists are not parsed yet.
#pragma once

#include <cstdint>

#include "evw/gamefiles/resource_data.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
    struct NavMesh : public ResourceFileBase
    {
        uint32_t contentFlags = 0;
        math::Vector3 aabbSize;
        uint32_t verticesCount = 0;
        uint32_t polysCount = 0;
        uint32_t areaID = 0;       // X + Y*100
        uint32_t totalBytes = 0;
        uint32_t pointsCount = 0;

        void read(ResourceDataReader& r);
    };
}
