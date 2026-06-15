// Port of GtxdFile.cs (RBF path) — the texture-dictionary parent mapping
// (gtxd.ymt as RBF "CMapParentTxds"). Builds a child-TXD -> parent-TXD map.
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace evw::gamefiles
{
    class GtxdFile
    {
    public:
        // Parses a gtxd RBF buffer. Returns false if it is not a CMapParentTxds RBF.
        bool loadRbf(const std::vector<uint8_t>& data);

        bool isLoaded() const { return loaded_; }
        const std::unordered_map<std::string, std::string>& relationships() const { return rels_; }

        // Returns the parent TXD of a child, or empty string.
        std::string parentOf(const std::string& child) const;

    private:
        bool loaded_ = false;
        std::unordered_map<std::string, std::string> rels_;
    };
}
