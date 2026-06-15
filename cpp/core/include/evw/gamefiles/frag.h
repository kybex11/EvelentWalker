// Port of the FragType header from Frag.cs (Yft). Parses the fragment header
// and its main visual Drawable; the physics LOD groups / glass / cloth are not
// parsed yet (pointers retained where relevant).
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "evw/gamefiles/drawable.h"
#include "evw/gamefiles/resource_data.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
    struct FragType : public ResourceFileBase
    {
        math::Vector3 boundingSphereCenter;
        float boundingSphereRadius = 0;
        uint64_t drawablePointer = 0;
        uint64_t namePointer = 0;
        float gravityFactor = 0;
        float buoyancyFactor = 0;
        uint8_t glassWindowsCount = 0;

        std::string name;
        std::shared_ptr<DrawableBase> drawable; // the main visual model

        void read(ResourceDataReader& r);
    };
}
