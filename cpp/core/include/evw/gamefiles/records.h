// Port of WaypointRecord.cs (Ywr) and VehicleRecord.cs (Yvr) — simple resource
// lists of positional/recording entries.
#pragma once

#include <cstdint>
#include <vector>

#include "evw/gamefiles/resource_data.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
    struct WaypointRecordEntry
    {
        math::Vector3 position;
        uint16_t unk0 = 0, unk1 = 0, unk2 = 0, unk3 = 0;
        void read(ResourceDataReader& r)
        {
            position = r.ReadVector3();
            unk0 = r.ReadUInt16(); unk1 = r.ReadUInt16();
            unk2 = r.ReadUInt16(); unk3 = r.ReadUInt16();
        }
    };

    struct WaypointRecordList : public ResourceFileBase
    {
        uint64_t entriesPointer = 0;
        uint32_t entriesCount = 0;
        std::vector<WaypointRecordEntry> entries;
        void read(ResourceDataReader& r);
    };

    struct VehicleRecordEntry
    {
        uint32_t time = 0;
        int16_t velocityX = 0, velocityY = 0, velocityZ = 0;
        int8_t rightX = 0, rightY = 0, rightZ = 0;
        int8_t forwardX = 0, forwardY = 0, forwardZ = 0;
        int8_t steeringAngle = 0, gasPedalPower = 0, brakePedalPower = 0;
        uint8_t handbrakeUsed = 0;
        math::Vector3 position;

        void read(ResourceDataReader& r)
        {
            time = r.ReadUInt32();
            velocityX = r.ReadInt16(); velocityY = r.ReadInt16(); velocityZ = r.ReadInt16();
            rightX = static_cast<int8_t>(r.ReadByte()); rightY = static_cast<int8_t>(r.ReadByte());
            rightZ = static_cast<int8_t>(r.ReadByte());
            forwardX = static_cast<int8_t>(r.ReadByte()); forwardY = static_cast<int8_t>(r.ReadByte());
            forwardZ = static_cast<int8_t>(r.ReadByte());
            steeringAngle = static_cast<int8_t>(r.ReadByte());
            gasPedalPower = static_cast<int8_t>(r.ReadByte());
            brakePedalPower = static_cast<int8_t>(r.ReadByte());
            handbrakeUsed = r.ReadByte();
            position = r.ReadVector3();
        }

        math::Vector3 velocity() const
        {
            return {velocityX / 273.0583f, velocityY / 273.0583f, velocityZ / 273.0583f};
        }
    };

    struct VehicleRecordList : public ResourceFileBase
    {
        ResourceSimpleList64<VehicleRecordEntry> entries;
        void read(ResourceDataReader& r);
    };
}
