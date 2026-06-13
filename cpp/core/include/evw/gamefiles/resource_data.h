// Port of EvelentWalker.Core/GameFiles/Resources/ResourceData.cs (read path).
// A resource file is split into a "system" segment and a "graphics" segment,
// addressed by virtual pointers with base 0x50000000 (system) / 0x60000000
// (graphics). ResourceDataReader resolves those pointers into the two buffers.
#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "evw/gamefiles/data.h"
#include "evw/math/math.h"

namespace evw::gamefiles
{
    class ResourceDataReader
    {
    public:
        static constexpr uint64_t SYSTEM_BASE = 0x50000000;
        static constexpr uint64_t GRAPHICS_BASE = 0x60000000;

        // Splits `data` into system[0..systemSize) and graphics[systemSize..+graphicsSize).
        ResourceDataReader(uint32_t systemSize, uint32_t graphicsSize,
                           const std::vector<uint8_t>& data,
                           Endianess endianess = Endianess::LittleEndian);

        uint64_t position() const { return position_; }
        void setPosition(uint64_t pos) { position_ = pos; }
        Endianess endianess() const { return endianess_; }

        uint8_t ReadByte();
        std::vector<uint8_t> ReadBytes(int count);
        int16_t ReadInt16();
        uint16_t ReadUInt16();
        int32_t ReadInt32();
        uint32_t ReadUInt32();
        int64_t ReadInt64();
        uint64_t ReadUInt64();
        float ReadSingle();
        math::Vector3 ReadVector3();
        math::Vector4 ReadVector4();
        std::string ReadString();

        // Positioned reads (restore position afterwards), mirroring *At helpers.
        std::vector<uint8_t> ReadBytesAt(uint64_t position, uint32_t count);
        std::vector<uint16_t> ReadUshortsAt(uint64_t position, uint32_t count);
        std::vector<int16_t> ReadShortsAt(uint64_t position, uint32_t count);
        std::vector<uint32_t> ReadUintsAt(uint64_t position, uint32_t count);
        std::vector<uint64_t> ReadUlongsAt(uint64_t position, uint32_t count);
        std::vector<float> ReadFloatsAt(uint64_t position, uint32_t count);
        std::string ReadStringAt(uint64_t position);

        // Reads `count` trivially-copyable structs of type T at the given pointer
        // (restores position). Returns empty if position is 0 or count is 0.
        template <class T>
        std::vector<T> ReadStructsAt(uint64_t position, uint32_t count)
        {
            std::vector<T> result;
            if (position == 0 || count == 0) return result;
            auto bytes = ReadBytesAt(position, static_cast<uint32_t>(count * sizeof(T)));
            result.resize(count);
            if (!bytes.empty()) std::memcpy(result.data(), bytes.data(), count * sizeof(T));
            return result;
        }

        // Reads a block of type T at the given pointer (0 => default), restoring
        // position afterwards. T must expose `void read(ResourceDataReader&)`.
        template <class T>
        std::shared_ptr<T> ReadBlockAt(uint64_t position)
        {
            if (position == 0) return nullptr;
            uint64_t backup = position_;
            position_ = position;
            auto result = std::make_shared<T>();
            result->read(*this);
            position_ = backup;
            return result;
        }

        template <class T>
        std::shared_ptr<T> ReadBlock()
        {
            auto result = std::make_shared<T>();
            result->read(*this);
            return result;
        }

    private:
        void readRaw(uint8_t* out, int count, bool ignoreEndianess = false);

        std::vector<uint8_t> system_;
        std::vector<uint8_t> graphics_;
        uint64_t position_ = SYSTEM_BASE;
        Endianess endianess_;
    };

    // ---- Resource file header blocks (ResourceFile.cs) ----

    struct ResourcePagesInfo
    {
        uint32_t unknown0 = 0;
        uint32_t unknown4 = 0;
        uint8_t systemPagesCount = 0;
        uint8_t graphicsPagesCount = 0;
        uint16_t unknownA = 0;
        uint32_t unknownC = 0;

        void read(ResourceDataReader& r);
    };

    struct ResourceFileBase
    {
        uint32_t fileVFT = 0;
        uint32_t fileUnknown = 1;
        uint64_t filePagesInfoPointer = 0;
        std::shared_ptr<ResourcePagesInfo> filePagesInfo;

        void read(ResourceDataReader& r);
    };

    // ---- Resource array base types (ResourceBaseTypes.cs) ----
    // Each element type T must expose `void read(ResourceDataReader&)`.

    // Contiguous array of N value-blocks read sequentially at the current position.
    template <class T>
    struct ResourceSimpleArray
    {
        std::vector<T> data;
        void read(ResourceDataReader& r, uint32_t numElements)
        {
            data.clear();
            data.resize(numElements);
            for (uint32_t i = 0; i < numElements; ++i) data[i].read(r);
        }
        size_t size() const { return data.size(); }
        const T& operator[](size_t i) const { return data[i]; }
        T& operator[](size_t i) { return data[i]; }
    };

    // 16-byte header {ptr,count,cap,pad} pointing to a contiguous array of value-blocks.
    template <class T>
    struct ResourceSimpleList64
    {
        uint64_t entriesPointer = 0;
        uint16_t entriesCount = 0;
        uint16_t entriesCapacity = 0;
        std::vector<T> items;

        void read(ResourceDataReader& r)
        {
            entriesPointer = r.ReadUInt64();
            entriesCount = r.ReadUInt16();
            entriesCapacity = r.ReadUInt16();
            r.setPosition(r.position() + 4);

            items.clear();
            items.resize(entriesCount);
            if (entriesCount > 0 && entriesPointer != 0)
            {
                uint64_t backup = r.position();
                r.setPosition(entriesPointer);
                for (uint16_t i = 0; i < entriesCount; ++i) items[i].read(r);
                r.setPosition(backup);
            }
        }
        size_t size() const { return items.size(); }
        const T& operator[](size_t i) const { return items[i]; }
    };

    // Array of N pointers (read at current position); each pointer dereferenced to a block.
    template <class T>
    struct ResourcePointerArray64
    {
        std::vector<uint64_t> pointers;
        std::vector<std::shared_ptr<T>> items;

        void read(ResourceDataReader& r, uint32_t numElements)
        {
            pointers = r.ReadUlongsAt(r.position(), numElements);
            items.assign(numElements, nullptr);
            for (uint32_t i = 0; i < numElements; ++i)
                items[i] = r.ReadBlockAt<T>(pointers[i]);
        }
        size_t size() const { return items.size(); }
        const std::shared_ptr<T>& operator[](size_t i) const { return items[i]; }
    };

    // 16-byte header pointing to a contiguous array of uint32 values.
    struct ResourceSimpleList64_uint
    {
        uint64_t entriesPointer = 0;
        uint16_t entriesCount = 0;
        uint16_t entriesCapacity = 0;
        std::vector<uint32_t> items;

        void read(ResourceDataReader& r)
        {
            entriesPointer = r.ReadUInt64();
            entriesCount = r.ReadUInt16();
            entriesCapacity = r.ReadUInt16();
            r.setPosition(r.position() + 4);
            items = r.ReadUintsAt(entriesPointer, entriesCount);
        }
        size_t size() const { return items.size(); }
    };

    // 16-byte header {ptr,count,cap,pad} pointing to an array of pointers to blocks.
    template <class T>
    struct ResourcePointerList64
    {
        uint64_t entriesPointer = 0;
        uint16_t entriesCount = 0;
        uint16_t entriesCapacity = 0;
        std::vector<uint64_t> pointers;
        std::vector<std::shared_ptr<T>> items;

        void read(ResourceDataReader& r)
        {
            entriesPointer = r.ReadUInt64();
            entriesCount = r.ReadUInt16();
            entriesCapacity = r.ReadUInt16();
            r.setPosition(r.position() + 4);

            pointers = r.ReadUlongsAt(entriesPointer, entriesCapacity);
            items.assign(entriesCount, nullptr);
            for (uint16_t i = 0; i < entriesCount; ++i)
                items[i] = r.ReadBlockAt<T>(i < pointers.size() ? pointers[i] : 0);
        }
        size_t size() const { return items.size(); }
        const std::shared_ptr<T>& operator[](size_t i) const { return items[i]; }
    };
}
