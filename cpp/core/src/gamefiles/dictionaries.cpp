#include "evw/gamefiles/dictionaries.h"

namespace evw::gamefiles
{
    namespace
    {
        // Reads a SimpleList64 of uint hashes at the current position.
        std::vector<uint32_t> readHashList(ResourceDataReader& r)
        {
            ResourceSimpleList64_uint list;
            list.read(r);
            return list.items;
        }

        // Reads a PointerList64 header at the current position and returns its
        // item count (without dereferencing the element blocks).
        uint16_t readPointerListCount(ResourceDataReader& r)
        {
            r.ReadUInt64();                 // entriesPointer
            uint16_t count = r.ReadUInt16();
            r.ReadUInt16();                 // capacity
            r.ReadUInt32();                 // pad
            return count;
        }
    }

    void ExpressionDictionary::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);
        r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32(); // Unknown_10h..1Ch
        nameHashes = readHashList(r);
        itemCount = readPointerListCount(r);
    }

    void FrameFilterDictionary::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);
        r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32(); r.ReadUInt32(); // Unknown_10h..1Ch
        nameHashes = readHashList(r);
        itemCount = readPointerListCount(r);
    }

    void ClothDictionary::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);
        r.ReadUInt64();                 // Unknown_10h
        r.ReadUInt64();                 // Unknown_18h
        nameHashes = readHashList(r);
        itemCount = readPointerListCount(r);
    }
}
