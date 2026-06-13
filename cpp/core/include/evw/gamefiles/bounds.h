// Port of the Bounds (collision) header from Bounds.cs. The base Bounds block
// holds the bounding box/sphere, material and type that are common to every
// bound kind. Specific bound subtypes (Composite/Geometry/BVH) are not parsed yet.
#pragma once

#include <cstdint>

#include "evw/gamefiles/resource_data.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
    enum class BoundsType : uint8_t
    {
        Sphere = 0,
        Capsule = 1,
        Box = 3,
        Geometry = 4,
        GeometryBVH = 8,
        Composite = 10,
        Disc = 12,
        Cylinder = 13,
        Cloth = 15,
        None = 255,
    };

    struct Bounds : public ResourceFileBase
    {
        BoundsType type = BoundsType::None;
        float sphereRadius = 0;
        math::Vector3 boxMax;
        float margin = 0;
        math::Vector3 boxMin;
        math::Vector3 boxCenter;
        uint8_t materialIndex = 0;
        uint8_t proceduralId = 0;
        uint8_t roomId = 0;
        uint8_t unkFlags = 0;
        math::Vector3 sphereCenter;
        uint8_t polyFlags = 0;
        uint8_t materialColorIndex = 0;
        float volume = 0;

        void read(ResourceDataReader& r);
    };

    // Composite bound: a tree of child bounds with per-child transforms.
    struct BoundComposite : public Bounds
    {
        uint16_t childrenCount = 0;
        ResourcePointerArray64<Bounds> children;
        std::vector<math::Matrix> childTransforms;

        void read(ResourceDataReader& r);
    };

    // Geometry bound: a triangle-soup collision mesh (quantized vertices).
    struct BoundGeometry : public Bounds
    {
        math::Vector3 quantum;
        math::Vector3 centerGeom;
        uint32_t verticesCount = 0;
        uint32_t polygonsCount = 0;
        uint8_t materialsCount = 0;
        std::vector<math::Vector3> vertices;  // dequantized (vertex * quantum)

        void read(ResourceDataReader& r);
    };
}
