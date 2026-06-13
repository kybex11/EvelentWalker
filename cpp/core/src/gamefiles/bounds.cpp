#include "evw/gamefiles/bounds.h"

#include <cstring>

namespace evw::gamefiles
{
    void Bounds::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        type = static_cast<BoundsType>(r.ReadByte());
        r.ReadByte();                   // Unknown_11h
        r.ReadUInt16();                 // Unknown_12h
        sphereRadius = r.ReadSingle();
        r.ReadUInt32();                 // Unknown_18h
        r.ReadUInt32();                 // Unknown_1Ch
        boxMax = r.ReadVector3();
        margin = r.ReadSingle();
        boxMin = r.ReadVector3();
        r.ReadUInt32();                 // Unknown_3Ch
        boxCenter = r.ReadVector3();
        materialIndex = r.ReadByte();
        proceduralId = r.ReadByte();
        roomId = r.ReadByte();
        unkFlags = r.ReadByte();
        sphereCenter = r.ReadVector3();
        polyFlags = r.ReadByte();
        materialColorIndex = r.ReadByte();
        r.ReadUInt16();                 // Unknown_5Eh
        r.ReadVector3();                // Unknown_60h
        volume = r.ReadSingle();
    }

    void BoundComposite::read(ResourceDataReader& r)
    {
        Bounds::read(r);

        uint64_t childrenPointer = r.ReadUInt64();
        uint64_t ct1Pointer = r.ReadUInt64();
        r.ReadUInt64();                 // ChildrenTransformation2Pointer
        r.ReadUInt64();                 // ChildrenBoundingBoxesPointer
        r.ReadUInt64();                 // ChildrenFlags1Pointer
        r.ReadUInt64();                 // ChildrenFlags2Pointer
        childrenCount = r.ReadUInt16();
        r.ReadUInt16();                 // ChildrenCount2
        r.ReadUInt32();                 // Unknown_A4h
        r.ReadUInt64();                 // BVHPointer

        if (childrenPointer != 0 && childrenCount != 0)
        {
            uint64_t backup = r.position();
            r.setPosition(childrenPointer);
            children.read(r, childrenCount);
            r.setPosition(backup);
        }
        childTransforms = r.ReadStructsAt<math::Matrix>(ct1Pointer, childrenCount);
    }

    void BoundGeometry::read(ResourceDataReader& r)
    {
        Bounds::read(r);

        r.ReadUInt32();                 // Unknown_70h
        r.ReadUInt32();                 // Unknown_74h
        r.ReadUInt64();                 // VerticesShrunkPointer
        r.ReadUInt16();                 // Unknown_80h
        r.ReadUInt16();                 // Unknown_82h
        r.ReadUInt32();                 // VerticesShrunkCount
        r.ReadUInt64();                 // PolygonsPointer
        quantum = r.ReadVector3();
        r.ReadSingle();                 // Unknown_9Ch
        centerGeom = r.ReadVector3();
        r.ReadSingle();                 // Unknown_ACh
        uint64_t verticesPointer = r.ReadUInt64();
        r.ReadUInt64();                 // VertexColoursPointer
        r.ReadUInt64();                 // OctantsPointer
        r.ReadUInt64();                 // OctantItemsPointer
        verticesCount = r.ReadUInt32();
        polygonsCount = r.ReadUInt32();
        for (int i = 0; i < 6; ++i) r.ReadUInt32(); // Unknown_D8h..ECh
        r.ReadUInt64();                 // MaterialsPointer
        r.ReadUInt64();                 // MaterialColoursPointer
        for (int i = 0; i < 6; ++i) r.ReadUInt32(); // Unknown_100h..114h
        r.ReadUInt64();                 // PolygonMaterialIndicesPointer
        materialsCount = r.ReadByte();
        r.ReadByte();                   // MaterialColoursCount
        r.ReadUInt16();                 // Unknown_122h
        r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32(); // 124/128/12C

        // Vertices are quantized int16 triples; dequantize by the quantum.
        if (verticesPointer != 0 && verticesCount != 0)
        {
            auto raw = r.ReadBytesAt(verticesPointer, verticesCount * 6);
            if (!raw.empty())
            {
                vertices.resize(verticesCount);
                for (uint32_t i = 0; i < verticesCount; ++i)
                {
                    int16_t x, y, z;
                    std::memcpy(&x, raw.data() + i * 6 + 0, 2);
                    std::memcpy(&y, raw.data() + i * 6 + 2, 2);
                    std::memcpy(&z, raw.data() + i * 6 + 4, 2);
                    vertices[i] = {x * quantum.X, y * quantum.Y, z * quantum.Z};
                }
            }
        }
    }
}
