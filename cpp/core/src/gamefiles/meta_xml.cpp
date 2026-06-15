#include "evw/gamefiles/meta_xml.h"

#include <cstring>

#include "evw/gamefiles/jenk.h"
#include "evw/gamefiles/metatypes.h"
#include "evw/gamefiles/xml_builder.h"

namespace evw::gamefiles
{
    namespace
    {
        using DT = MetaStructureEntryDataType;

        template <class T>
        T readPod(const std::vector<uint8_t>& data, size_t off)
        {
            T v{};
            if (off + sizeof(T) <= data.size()) std::memcpy(&v, data.data() + off, sizeof(T));
            return v;
        }

        std::string enumName(const Meta& meta, int16_t refIndex, int32_t value)
        {
            if (refIndex >= 0 && refIndex < static_cast<int16_t>(meta.enumInfos.size()))
            {
                for (const auto& e : meta.enumInfos.data[refIndex].entries)
                    if (e.entryValue == value) return metaHashName(e.entryNameHash);
            }
            return std::to_string(value);
        }

        // Reads a null-terminated string from a data block referenced by a meta
        // pointer/char-array descriptor (block index + offset).
        std::string readCharArray(const Meta& meta, uint64_t pointer)
        {
            uint32_t bidx = metatypes::pointerBlockIndex(pointer);
            if (bidx == 0) return {};
            --bidx;
            if (bidx >= meta.dataBlocks.size()) return {};
            const auto& blk = meta.dataBlocks.data[bidx];
            uint32_t off = metatypes::pointerOffset(pointer);
            std::string s;
            for (size_t i = off; i < blk.data.size(); ++i)
            {
                if (blk.data[i] == 0) break;
                s.push_back(static_cast<char>(blk.data[i]));
            }
            return s;
        }

        void serializeStructure(const Meta& meta, uint32_t structHash,
                                const std::vector<uint8_t>& data, size_t baseOffset,
                                int level, xml::Writer& w);

        // Serializes a single structure entry (field) at the given base offset.
        void serializeEntry(const Meta& meta, const MetaStructureEntryInfo_s& e,
                            const std::vector<uint8_t>& data, size_t baseOffset,
                            int level, xml::Writer& w)
        {
            const std::string tag = metaHashName(e.entryNameHash);
            const size_t off = baseOffset + static_cast<size_t>(e.dataOffset);

            switch (static_cast<DT>(e.dataType))
            {
            case DT::Boolean:
                w.valueTag(level, tag, readPod<uint8_t>(data, off) ? "true" : "false");
                break;
            case DT::SignedByte:
                w.valueTag(level, tag, std::to_string(static_cast<int>(readPod<int8_t>(data, off))));
                break;
            case DT::UnsignedByte:
                w.valueTag(level, tag, std::to_string(static_cast<int>(readPod<uint8_t>(data, off))));
                break;
            case DT::SignedShort:
                w.valueTag(level, tag, std::to_string(readPod<int16_t>(data, off)));
                break;
            case DT::UnsignedShort:
                w.valueTag(level, tag, std::to_string(readPod<uint16_t>(data, off)));
                break;
            case DT::SignedInt:
                w.valueTag(level, tag, std::to_string(readPod<int32_t>(data, off)));
                break;
            case DT::UnsignedInt:
                w.valueTag(level, tag, std::to_string(readPod<uint32_t>(data, off)));
                break;
            case DT::Float:
                w.valueTag(level, tag, xml::fmtFloat(readPod<float>(data, off)));
                break;
            case DT::Float_XYZ:
            {
                float x = readPod<float>(data, off), y = readPod<float>(data, off + 4), z = readPod<float>(data, off + 8);
                w.selfClose(level, tag, "x=\"" + xml::fmtFloat(x) + "\" y=\"" + xml::fmtFloat(y) + "\" z=\"" + xml::fmtFloat(z) + "\"");
                break;
            }
            case DT::Float_XYZW:
            {
                float x = readPod<float>(data, off), y = readPod<float>(data, off + 4),
                      z = readPod<float>(data, off + 8), wv = readPod<float>(data, off + 12);
                w.selfClose(level, tag, "x=\"" + xml::fmtFloat(x) + "\" y=\"" + xml::fmtFloat(y) +
                                        "\" z=\"" + xml::fmtFloat(z) + "\" w=\"" + xml::fmtFloat(wv) + "\"");
                break;
            }
            case DT::Hash:
                w.stringTag(level, tag, metaHashName(readPod<uint32_t>(data, off)));
                break;
            case DT::ByteEnum:
                w.valueTag(level, tag, enumName(meta, e.referenceTypeIndex, readPod<uint8_t>(data, off)));
                break;
            case DT::IntEnum:
                w.valueTag(level, tag, enumName(meta, e.referenceTypeIndex, readPod<int32_t>(data, off)));
                break;
            case DT::ShortFlags:
                w.valueTag(level, tag, std::to_string(readPod<uint16_t>(data, off)));
                break;
            case DT::IntFlags1:
            case DT::IntFlags2:
                w.valueTag(level, tag, std::to_string(readPod<uint32_t>(data, off)));
                break;
            case DT::ArrayOfChars:
            case DT::CharPointer:
            {
                uint64_t ptr = readPod<uint64_t>(data, off);
                w.stringTag(level, tag, readCharArray(meta, ptr));
                break;
            }
            case DT::ArrayOfBytes:
            {
                metatypes::MetaArray arr = readPod<metatypes::MetaArray>(data, off);
                w.selfClose(level, tag, "bytes=\"" + std::to_string(arr.count1) + "\"");
                break;
            }
            case DT::DataBlockPointer:
                w.selfClose(level, tag, "block=\"" + std::to_string(readPod<uint32_t>(data, off) & 0xFFF) + "\"");
                break;
            case DT::Structure:
            {
                // Inline structure: recurse at the same block, referenced by index.
                int16_t idx = e.referenceTypeIndex;
                if (idx >= 0 && idx < static_cast<int16_t>(meta.structureInfos.size()))
                {
                    w.open(level, tag);
                    serializeStructure(meta, meta.structureInfos.data[idx].structureNameHash, data, off, level + 1, w);
                    w.close(level, tag);
                }
                else w.comment(level, tag + " (unresolved inline structure)");
                break;
            }
            case DT::StructurePointer:
            {
                metatypes::MetaPointer ptr = readPod<metatypes::MetaPointer>(data, off);
                uint32_t bidx = ptr.blockId();
                if (bidx == 0) { w.selfClose(level, tag, "ref=\"null\""); break; }
                --bidx;
                if (bidx < meta.dataBlocks.size())
                {
                    const auto& blk = meta.dataBlocks.data[bidx];
                    w.open(level, tag);
                    serializeStructure(meta, blk.structureNameHash, blk.data, ptr.offset(), level + 1, w);
                    w.close(level, tag);
                }
                else w.comment(level, tag + " (dangling structure pointer)");
                break;
            }
            case DT::Array:
            {
                metatypes::MetaArray arr = readPod<metatypes::MetaArray>(data, off);
                uint32_t count = arr.count1;
                int16_t idx = e.referenceTypeIndex;
                if (count > 0 && idx >= 0 && idx < static_cast<int16_t>(meta.structureInfos.size()))
                {
                    const MetaStructureInfo& si = meta.structureInfos.data[idx];
                    uint32_t bidx = arr.blockId();
                    if (bidx > 0 && (bidx - 1) < meta.dataBlocks.size())
                    {
                        const auto& blk = meta.dataBlocks.data[bidx - 1];
                        w.open(level, tag);
                        for (uint32_t i = 0; i < count; ++i)
                        {
                            size_t eo = arr.offset() + static_cast<size_t>(i) * si.structureSize;
                            w.open(level + 1, "Item");
                            serializeStructure(meta, si.structureNameHash, blk.data, eo, level + 2, w);
                            w.close(level + 1, "Item");
                        }
                        w.close(level, tag);
                        break;
                    }
                }
                w.selfClose(level, tag, "count=\"" + std::to_string(count) + "\"");
                break;
            }
            default:
                w.comment(level, tag + " (type 0x" + [&] {
                    char b[8]; std::snprintf(b, sizeof(b), "%02X", e.dataType); return std::string(b);
                }() + " not serialized)");
                break;
            }
        }

        void serializeStructure(const Meta& meta, uint32_t structHash,
                                const std::vector<uint8_t>& data, size_t baseOffset,
                                int level, xml::Writer& w)
        {
            const MetaStructureInfo* si = meta.findStructure(structHash);
            if (!si)
            {
                w.comment(level, "unknown structure " + metaHashName(structHash));
                return;
            }
            for (const auto& e : si->entries)
            {
                // Skip the array-holder pseudo entry (offset 0, used internally
                // by some array structures) only if it duplicates; we serialize
                // all declared entries otherwise.
                serializeEntry(meta, e, data, baseOffset, level, w);
            }
        }
    }

    std::string metaHashName(uint32_t hash)
    {
        std::string s = JenkIndex::TryGetString(hash);
        if (!s.empty())
        {
            // Must be a valid XML tag start (letter or underscore).
            char c = s[0];
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') return s;
        }
        return "hash_" + std::to_string(hash);
    }

    std::string metaToXml(const Meta& meta)
    {
        xml::Writer w;
        w.header();

        const MetaDataBlock* root = meta.rootBlock();
        if (!root)
        {
            w.open(0, "Meta");
            w.comment(1, "no root block");
            w.close(0, "Meta");
            return w.str();
        }

        const std::string rootTag = metaHashName(root->structureNameHash);
        w.open(0, rootTag);
        serializeStructure(meta, root->structureNameHash, root->data, 0, 1, w);
        w.close(0, rootTag);
        return w.str();
    }
}
