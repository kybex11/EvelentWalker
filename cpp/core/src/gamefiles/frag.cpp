#include "evw/gamefiles/frag.h"

namespace evw::gamefiles
{
    void FragType::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        r.ReadUInt64();                 // Unknown_10h
        r.ReadUInt64();                 // Unknown_18h
        boundingSphereCenter = r.ReadVector3();
        boundingSphereRadius = r.ReadSingle();
        drawablePointer = r.ReadUInt64();
        r.ReadUInt64();                 // DrawableArrayPointer
        r.ReadUInt64();                 // DrawableArrayNamesPointer
        r.ReadUInt32();                 // DrawableArrayCount
        r.ReadInt32();                  // DrawableArrayFlag
        r.ReadUInt64();                 // Unknown_50h
        namePointer = r.ReadUInt64();
        // Cloths (ResourcePointerList64) header — skip 16 bytes.
        r.ReadUInt64(); r.ReadUInt16(); r.ReadUInt16(); r.ReadUInt32();
        for (int i = 0; i < 7; ++i) r.ReadUInt64(); // Unknown_70h..A0h
        r.ReadUInt64();                 // BoneTransformsPointer
        for (int i = 0; i < 7; ++i) r.ReadInt32();  // Unknown_B0h..C8h
        r.ReadSingle();                 // Unknown_CCh
        gravityFactor = r.ReadSingle();
        buoyancyFactor = r.ReadSingle();
        r.ReadByte();                   // Unknown_D8h
        glassWindowsCount = r.ReadByte();
        r.ReadUInt16();                 // Unknown_DAh
        r.ReadUInt32();                 // Unknown_DCh
        r.ReadUInt64();                 // GlassWindowsPointer
        r.ReadUInt64();                 // Unknown_E8h
        r.ReadUInt64();                 // PhysicsLODGroupPointer
        r.ReadUInt64();                 // DrawableClothPointer
        r.ReadUInt64();                 // Unknown_100h
        r.ReadUInt64();                 // Unknown_108h
        // LightAttributes (ResourceSimpleList64) header — skip 16 bytes.
        r.ReadUInt64(); r.ReadUInt16(); r.ReadUInt16(); r.ReadUInt32();
        r.ReadUInt64();                 // VehicleGlassWindowsPointer
        r.ReadUInt64();                 // Unknown_128h

        drawable = r.ReadBlockAt<DrawableBase>(drawablePointer);
        name = r.ReadStringAt(namePointer);
    }
}
