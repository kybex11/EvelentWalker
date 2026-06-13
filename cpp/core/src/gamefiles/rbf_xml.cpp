#include "evw/gamefiles/rbf_xml.h"

#include <cstdio>
#include <sstream>

namespace evw::gamefiles
{
    namespace
    {
        std::string escape(const std::string& s)
        {
            std::string out;
            for (char c : s)
            {
                switch (c)
                {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '"': out += "&quot;"; break;
                default: out += c; break;
                }
            }
            return out;
        }

        void writeNode(std::ostringstream& os, const std::shared_ptr<RbfNode>& n, int indent)
        {
            if (!n) return;
            std::string pad(static_cast<size_t>(indent) * 2, ' ');
            const std::string& tag = n->name.empty() ? std::string("Item") : n->name;
            char buf[64];

            switch (n->type)
            {
            case RbfType::Structure:
                os << pad << "<" << tag << ">\n";
                for (const auto& c : n->children) writeNode(os, c, indent + 1);
                os << pad << "</" << tag << ">\n";
                break;
            case RbfType::Uint32:
                os << pad << "<" << tag << " value=\"" << n->u32 << "\" />\n";
                break;
            case RbfType::Boolean:
                os << pad << "<" << tag << " value=\"" << (n->boolean ? "true" : "false") << "\" />\n";
                break;
            case RbfType::Float:
                std::snprintf(buf, sizeof(buf), "%g", n->f);
                os << pad << "<" << tag << " value=\"" << buf << "\" />\n";
                break;
            case RbfType::Float3:
                std::snprintf(buf, sizeof(buf), "%g %g %g", n->x, n->y, n->z);
                os << pad << "<" << tag << " value=\"" << buf << "\" />\n";
                break;
            case RbfType::String:
                os << pad << "<" << tag << ">" << escape(n->str) << "</" << tag << ">\n";
                break;
            case RbfType::Bytes:
                os << pad << "<" << tag << " bytes=\"" << n->bytes.size() << "\" />\n";
                break;
            }
        }
    }

    std::string rbfToXml(const std::shared_ptr<RbfNode>& root)
    {
        std::ostringstream os;
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        writeNode(os, root, 0);
        return os.str();
    }
}
