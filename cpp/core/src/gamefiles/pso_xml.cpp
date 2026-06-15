#include "evw/gamefiles/pso_xml.h"

#include "evw/gamefiles/meta_xml.h" // metaHashName for readable tag names
#include "evw/gamefiles/xml_builder.h"

namespace evw::gamefiles
{
    namespace
    {
        // PsoDataType values (see Pso.cs).
        enum PsoType : uint8_t
        {
            Bool = 0x00, SByte = 0x01, UByte = 0x02, SShort = 0x03, UShort = 0x04,
            SInt = 0x05, UInt = 0x06, Float = 0x07, Float2 = 0x08, Float3 = 0x09,
            Float4 = 0x0a, String = 0x0b, Structure = 0x0c, Array = 0x0d, Enum = 0x0e,
            Flags = 0x0f, Map = 0x10, Float3a = 0x14, Float4a = 0x15, HFloat = 0x1e, Long = 0x20,
        };

        std::string enumName(const PsoFile& pso, uint32_t enumKey, int32_t value)
        {
            const PsoSchemaEnum* en = pso.findEnum(enumKey);
            if (en)
                for (const auto& e : en->entries)
                    if (e.entryKey == value) return metaHashName(e.entryNameHash);
            return std::to_string(value);
        }

        void floatVec(const PsoFile& pso, xml::Writer& w, int level, const std::string& tag,
                      int32_t baseOff, int count)
        {
            std::string attrs;
            const char* names = "xyzw";
            for (int i = 0; i < count; ++i)
            {
                if (i) attrs += " ";
                attrs += std::string(1, names[i]) + "=\"" +
                         xml::fmtFloat(pso.readFloatAt(baseOff + i * 4)) + "\"";
            }
            w.selfClose(level, tag, attrs);
        }

        void serializeStructure(const PsoFile& pso, uint32_t structHash, int blockId,
                                uint32_t baseOffset, int level, xml::Writer& w)
        {
            const PsoSchemaStructure* s = pso.findSchema(structHash);
            if (!s)
            {
                w.comment(level, "unknown structure " + metaHashName(structHash));
                return;
            }
            for (const auto& e : s->entries)
            {
                const std::string tag = metaHashName(e.entryNameHash);
                const int32_t off = pso.fieldOffset(blockId, static_cast<uint16_t>(baseOffset + e.dataOffset));
                if (off < 0) continue;
                switch (e.type)
                {
                case Bool:   w.valueTag(level, tag, pso.readByteAt(off) ? "true" : "false"); break;
                case SByte:  w.valueTag(level, tag, std::to_string(static_cast<int8_t>(pso.readByteAt(off)))); break;
                case UByte:  w.valueTag(level, tag, std::to_string(static_cast<int>(pso.readByteAt(off)))); break;
                case SShort: w.valueTag(level, tag, std::to_string(static_cast<int16_t>(pso.readUInt16At(off)))); break;
                case UShort: w.valueTag(level, tag, std::to_string(pso.readUInt16At(off))); break;
                case SInt:   w.valueTag(level, tag, std::to_string(pso.readInt32At(off))); break;
                case UInt:   w.valueTag(level, tag, std::to_string(pso.readUInt32At(off))); break;
                case Float:  w.valueTag(level, tag, xml::fmtFloat(pso.readFloatAt(off))); break;
                case Float2: floatVec(pso, w, level, tag, off, 2); break;
                case Float3:
                case Float3a: floatVec(pso, w, level, tag, off, 3); break;
                case Float4:
                case Float4a: floatVec(pso, w, level, tag, off, 4); break;
                case Enum:   w.valueTag(level, tag, enumName(pso, e.referenceKey, pso.readInt32At(off))); break;
                case Flags:  w.valueTag(level, tag, std::to_string(pso.readUInt32At(off))); break;
                case Long:
                {
                    uint64_t hi = pso.readUInt32At(off);
                    uint64_t lo = pso.readUInt32At(off + 4);
                    w.valueTag(level, tag, std::to_string((hi << 32) | lo));
                    break;
                }
                case Structure:
                {
                    // Inline sub-structure within the same block, at this offset.
                    if (pso.findSchema(e.referenceKey))
                    {
                        w.open(level, tag);
                        serializeStructure(pso, e.referenceKey, blockId,
                                           baseOffset + e.dataOffset, level + 1, w);
                        w.close(level, tag);
                    }
                    else w.comment(level, tag + " (unresolved nested structure)");
                    break;
                }
                case String: w.comment(level, tag + " (string)"); break;
                case Array: w.comment(level, tag + " (array)"); break;
                case Map: w.comment(level, tag + " (map)"); break;
                case HFloat: w.comment(level, tag + " (half float)"); break;
                default: w.comment(level, tag + " (type not serialized)"); break;
                }
            }
        }
    }

    std::string psoToXml(const PsoFile& pso)
    {
        xml::Writer w;
        w.header();
        const PsoDataMappingEntry* root = pso.getBlock(pso.rootId());
        if (!root)
        {
            w.open(0, "PSO");
            w.comment(1, "no root block");
            w.close(0, "PSO");
            return w.str();
        }
        const std::string rootTag = metaHashName(root->nameHash);
        w.open(0, rootTag);
        serializeStructure(pso, root->nameHash, pso.rootId(), 0, 1, w);
        w.close(0, rootTag);
        return w.str();
    }
}
