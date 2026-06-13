// Port of Rbf.cs — the RBF binary-XML metadata format (little-endian). Parses
// into a tree of typed nodes (structure / uint / bool / float / float3 / string
// / bytes), mirroring the original RbfStructure hierarchy.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace evw::gamefiles
{
    enum class RbfType
    {
        Structure, Uint32, Boolean, Float, Float3, String, Bytes,
    };

    struct RbfNode
    {
        RbfType type = RbfType::Structure;
        std::string name;

        uint32_t u32 = 0;
        bool boolean = false;
        float f = 0;
        float x = 0, y = 0, z = 0;
        std::string str;
        std::vector<uint8_t> bytes;
        std::vector<std::shared_ptr<RbfNode>> children;

        // Finds the first child with the given name (or nullptr).
        std::shared_ptr<RbfNode> find(const std::string& childName) const;
    };

    class RbfFile
    {
    public:
        // Parses an RBF buffer; returns the root structure (or nullptr on error).
        std::shared_ptr<RbfNode> load(const std::vector<uint8_t>& data);

        std::shared_ptr<RbfNode> root() const { return root_; }

        static bool isRBF(const std::vector<uint8_t>& data);

    private:
        std::shared_ptr<RbfNode> root_;
    };
}
