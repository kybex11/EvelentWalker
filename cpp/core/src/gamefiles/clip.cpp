#include "evw/gamefiles/clip.h"

namespace evw::gamefiles
{
    void ClipDictionary::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        unknown10 = r.ReadUInt32();
        unknown14 = r.ReadUInt32();
        animationsPointer = r.ReadUInt64();
        unknown20 = r.ReadUInt32();
        unknown24 = r.ReadUInt32();
        clipsPointer = r.ReadUInt64();
        clipsMapCapacity = r.ReadUInt16();
        clipsMapEntries = r.ReadUInt16();
        unknown34 = r.ReadUInt32();
        unknown38 = r.ReadUInt32();
        unknown3C = r.ReadUInt32();

        // The clips map is a hash table of `capacity` pointer slots.
        clipPointers.clear();
        if (clipsPointer != 0 && clipsMapCapacity > 0)
            clipPointers = r.ReadUlongsAt(clipsPointer, clipsMapCapacity);
    }
}
