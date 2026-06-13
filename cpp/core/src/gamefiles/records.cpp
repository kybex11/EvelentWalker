#include "evw/gamefiles/records.h"

namespace evw::gamefiles
{
    void WaypointRecordList::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);

        r.ReadUInt32();                 // Unknown_10h
        r.ReadUInt32();                 // Unknown_14h
        entriesPointer = r.ReadUInt64();
        entriesCount = r.ReadUInt32();
        r.ReadUInt32();                 // Unknown_24h
        r.ReadUInt32();                 // Unknown_28h
        r.ReadUInt32();                 // Unknown_2Ch

        entries.clear();
        if (entriesPointer != 0 && entriesCount != 0)
        {
            uint64_t backup = r.position();
            r.setPosition(entriesPointer);
            entries.resize(entriesCount);
            for (uint32_t i = 0; i < entriesCount; ++i) entries[i].read(r);
            r.setPosition(backup);
        }
    }

    void VehicleRecordList::read(ResourceDataReader& r)
    {
        ResourceFileBase::read(r);
        entries.read(r);
    }
}
