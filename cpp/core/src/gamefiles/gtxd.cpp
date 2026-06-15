#include "evw/gamefiles/gtxd.h"

#include "evw/gamefiles/rbf.h"

namespace evw::gamefiles
{
    namespace
    {
        // Extracts an ASCII string from a "parent"/"child" wrapper structure whose
        // first child is a Bytes node (trailing nulls trimmed).
        std::string strFromWrapper(const std::shared_ptr<RbfNode>& node)
        {
            if (!node || node->children.empty()) return {};
            const auto& bytes = node->children[0]->bytes;
            std::string s;
            for (uint8_t b : bytes)
            {
                if (b == 0) continue;
                s.push_back(static_cast<char>(b));
            }
            return s;
        }
    }

    bool GtxdFile::loadRbf(const std::vector<uint8_t>& data)
    {
        loaded_ = false;
        rels_.clear();

        RbfFile rbf;
        auto root = rbf.load(data);
        if (!root || root->name != "CMapParentTxds") return false;

        auto txdRels = root->find("txdRelationships");
        if (!txdRels) { loaded_ = true; return true; } // valid but empty

        for (const auto& item : txdRels->children)
        {
            if (!item || item->name != "item") continue;
            std::string parent, child;
            for (const auto& field : item->children)
            {
                if (!field) continue;
                if (field->name == "parent") parent = strFromWrapper(field);
                else if (field->name == "child") child = strFromWrapper(field);
            }
            if (!parent.empty() && !child.empty())
                rels_.emplace(child, parent); // first wins, like the original
        }

        loaded_ = true;
        return true;
    }

    std::string GtxdFile::parentOf(const std::string& child) const
    {
        auto it = rels_.find(child);
        return it != rels_.end() ? it->second : std::string();
    }
}
