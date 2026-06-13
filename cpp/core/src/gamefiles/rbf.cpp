#include "evw/gamefiles/rbf.h"

#include <stdexcept>

#include "evw/gamefiles/data.h"

namespace evw::gamefiles
{
    namespace
    {
        constexpr int32_t RBF_IDENT = 0x30464252; // "RBF0"

        struct Descriptor { std::string name; uint8_t type; };
    }

    std::shared_ptr<RbfNode> RbfNode::find(const std::string& childName) const
    {
        for (const auto& c : children)
            if (c && c->name == childName) return c;
        return nullptr;
    }

    bool RbfFile::isRBF(const std::vector<uint8_t>& data)
    {
        if (data.size() < 4) return false;
        int32_t ident = static_cast<int32_t>(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
        return ident == RBF_IDENT;
    }

    std::shared_ptr<RbfNode> RbfFile::load(const std::vector<uint8_t>& data)
    {
        root_ = nullptr;
        if (!isRBF(data)) return nullptr;

        DataReader reader(data);
        std::vector<Descriptor> descriptors;
        std::vector<std::shared_ptr<RbfNode>> stack;
        std::shared_ptr<RbfNode> current;

        reader.ReadInt32(); // ident (validated)

        auto parseElement = [&](int descriptorIndex, uint8_t dataType) {
            const Descriptor& d = descriptors[descriptorIndex];
            switch (dataType)
            {
            case 0x00: // open structure
            {
                auto s = std::make_shared<RbfNode>();
                s->type = RbfType::Structure;
                s->name = d.name;
                if (current) { current->children.push_back(s); stack.push_back(current); }
                current = s;
                reader.ReadInt16(); // x1
                reader.ReadInt16(); // x2
                reader.ReadInt16(); // pending attributes
                break;
            }
            case 0x10:
            {
                auto n = std::make_shared<RbfNode>(); n->type = RbfType::Uint32; n->name = d.name;
                n->u32 = reader.ReadUInt32();
                if (current) current->children.push_back(n);
                break;
            }
            case 0x20:
            case 0x30:
            {
                auto n = std::make_shared<RbfNode>(); n->type = RbfType::Boolean; n->name = d.name;
                n->boolean = (dataType == 0x20);
                if (current) current->children.push_back(n);
                break;
            }
            case 0x40:
            {
                auto n = std::make_shared<RbfNode>(); n->type = RbfType::Float; n->name = d.name;
                n->f = reader.ReadSingle();
                if (current) current->children.push_back(n);
                break;
            }
            case 0x50:
            {
                auto n = std::make_shared<RbfNode>(); n->type = RbfType::Float3; n->name = d.name;
                n->x = reader.ReadSingle(); n->y = reader.ReadSingle(); n->z = reader.ReadSingle();
                if (current) current->children.push_back(n);
                break;
            }
            case 0x60:
            {
                auto n = std::make_shared<RbfNode>(); n->type = RbfType::String; n->name = d.name;
                int16_t len = reader.ReadInt16();
                auto b = reader.ReadBytes(len);
                n->str.assign(b.begin(), b.end());
                if (current) current->children.push_back(n);
                break;
            }
            default:
                throw std::runtime_error("RBF: unsupported data type");
            }
        };

        while (reader.position() < reader.length())
        {
            uint8_t descriptorIndex = reader.ReadByte();
            if (descriptorIndex == 0xFF) // close tag
            {
                uint8_t b = reader.ReadByte();
                if (b != 0xFF) throw std::runtime_error("RBF: expected 0xFF");
                if (!stack.empty()) { current = stack.back(); stack.pop_back(); }
                else { root_ = current; return root_; }
            }
            else if (descriptorIndex == 0xFD) // bytes
            {
                uint8_t b = reader.ReadByte();
                if (b != 0xFF) throw std::runtime_error("RBF: expected 0xFF");
                int32_t len = reader.ReadInt32();
                auto n = std::make_shared<RbfNode>(); n->type = RbfType::Bytes;
                n->bytes = reader.ReadBytes(len);
                if (current) current->children.push_back(n);
            }
            else
            {
                uint8_t dataType = reader.ReadByte();
                if (descriptorIndex == descriptors.size())
                {
                    int16_t nameLen = reader.ReadInt16();
                    auto nb = reader.ReadBytes(nameLen);
                    Descriptor d; d.name.assign(nb.begin(), nb.end()); d.type = dataType;
                    descriptors.push_back(d);
                    parseElement(static_cast<int>(descriptors.size()) - 1, dataType);
                }
                else
                {
                    parseElement(descriptorIndex, dataType);
                }
            }
        }
        return root_; // may be null on malformed input
    }
}
