// Port of the data-reading helpers from MetaTypes.cs.
// Meta pointers encode a 1-based data-block index in the low 12 bits and a byte
// offset within the block in the next 20 bits. These helpers resolve such
// pointers into typed POD values/arrays.
#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "evw/gamefiles/meta.h"

namespace evw::gamefiles::metatypes
{
#pragma pack(push, 1)
    // 16-byte array descriptor used inside meta structures (entities, etc.).
    struct MetaArray
    {
        uint64_t pointer = 0;
        uint16_t count1 = 0;
        uint16_t count2 = 0;
        uint32_t unknown = 0;

        uint32_t blockId() const { return static_cast<uint32_t>(pointer & 0xFFF); }
        uint32_t offset() const { return static_cast<uint32_t>((pointer >> 12) & 0xFFFFF); }
    };
    static_assert(sizeof(MetaArray) == 16, "MetaArray must be 16 bytes");

    // 8-byte pointer to a data item (block index + offset).
    struct MetaPointer
    {
        uint64_t value = 0;
        uint32_t blockId() const { return static_cast<uint32_t>(value & 0xFFF); }      // 1-based
        uint32_t offset() const { return static_cast<uint32_t>((value >> 12) & 0xFFFFF); }
    };
    static_assert(sizeof(MetaPointer) == 8, "MetaPointer must be 8 bytes");
#pragma pack(pop)

    inline uint32_t pointerBlockIndex(uint64_t pointer) { return static_cast<uint32_t>(pointer & 0xFFF); }       // 1-based
    inline uint32_t pointerOffset(uint64_t pointer) { return static_cast<uint32_t>((pointer >> 12) & 0xFFFFF); }

    // Reads `count` POD values of type T starting at the encoded pointer,
    // spanning consecutive data blocks if needed. Returns empty on mismatch.
    template <class T>
    std::vector<T> convertDataArray(const Meta& meta, uint32_t name, uint64_t pointer, uint32_t count)
    {
        std::vector<T> items;
        if (count == 0) return items;
        items.resize(count);

        const int itemsize = static_cast<int>(sizeof(T));
        int itemsleft = static_cast<int>(count);

        const auto& blocks = meta.dataBlocks.data;
        uint32_t ptrindex = pointerBlockIndex(pointer) - 1;
        uint32_t ptroffset = pointerOffset(pointer);

        const MetaDataBlock* block = (ptrindex < blocks.size()) ? &blocks[ptrindex] : nullptr;
        if (!block || block->data.empty() || block->structureNameHash != name)
            return {};

        int itemoffset = static_cast<int>(ptroffset) / itemsize;
        int curi = 0;
        while (itemsleft > 0)
        {
            int blockcount = static_cast<int>(block->data.size()) / itemsize;
            int itemcount = blockcount - itemoffset;
            if (itemcount > itemsleft) itemcount = itemsleft;
            for (int i = 0; i < itemcount; ++i)
            {
                int offset = (itemoffset + i) * itemsize;
                std::memcpy(&items[curi + i], block->data.data() + offset, itemsize);
            }
            itemoffset = 0;
            curi += itemcount;
            itemsleft -= itemcount;
            if (itemsleft <= 0) return items;

            ++ptrindex;
            block = (ptrindex < blocks.size()) ? &blocks[ptrindex] : nullptr;
            if (!block || block->data.empty() || block->structureNameHash != name) break;
        }
        return {};
    }

    // Reads a single POD value at the encoded pointer.
    template <class T>
    bool getDataAt(const Meta& meta, uint32_t name, uint64_t pointer, T& out)
    {
        auto v = convertDataArray<T>(meta, name, pointer, 1);
        if (v.empty()) return false;
        out = v[0];
        return true;
    }

    // Finds the first data block matching `name` and reads it as T.
    template <class T>
    bool getTypedData(const Meta& meta, uint32_t name, T& out)
    {
        for (const auto& block : meta.dataBlocks.data)
        {
            if (block.structureNameHash == name && block.data.size() >= sizeof(T))
            {
                std::memcpy(&out, block.data.data(), sizeof(T));
                return true;
            }
        }
        return false;
    }

    // Reads a structure-pointer array: a contiguous array of MetaPointer entries
    // located at `arrayPointer`, each dereferenced to a T of structure `name`.
    // If pointerBlockName != 0 it is checked against the pointer block's hash.
    template <class T>
    std::vector<T> convertPointerArray(const Meta& meta, uint32_t name,
                                       const MetaArray& array, uint32_t pointerBlockName = 0)
    {
        std::vector<T> items;
        uint32_t count = array.count1;
        if (count == 0) return items;

        const auto& blocks = meta.dataBlocks.data;
        uint32_t pidx = array.blockId() - 1;
        if (pidx >= blocks.size()) return {};
        const MetaDataBlock& pblock = blocks[pidx];
        if (pblock.data.empty()) return {};
        if (pointerBlockName != 0 && pblock.structureNameHash != pointerBlockName) return {};

        items.resize(count);
        uint32_t poff = array.offset();
        for (uint32_t i = 0; i < count; ++i)
        {
            size_t off = poff + i * sizeof(MetaPointer);
            if (off + sizeof(MetaPointer) > pblock.data.size()) break;
            MetaPointer ptr;
            std::memcpy(&ptr, pblock.data.data() + off, sizeof(MetaPointer));

            uint32_t bidx = ptr.blockId() - 1;
            if (bidx >= blocks.size()) continue;
            const MetaDataBlock& blk = blocks[bidx];
            if (blk.structureNameHash != name) return {};
            uint32_t boff = ptr.offset();
            if (boff + sizeof(T) > blk.data.size()) continue;
            std::memcpy(&items[i], blk.data.data() + boff, sizeof(T));
        }
        return items;
    }
}
