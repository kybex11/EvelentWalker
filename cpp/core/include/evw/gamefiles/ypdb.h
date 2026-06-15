// Port of YpdbFile.cs (header) — the pose-matcher database (.ypdb). A small
// little-endian binary file: a 0x1A "binary marker", versions, signature, and
// sample/bone-tag counts. This port reads the leading header fields.
#pragma once

#include <cstdint>
#include <vector>

namespace evw::gamefiles
{
    class YpdbFile
    {
    public:
        // Parses a .ypdb buffer. Returns false if too small or wrong marker.
        bool load(const std::vector<uint8_t>& data);

        bool isLoaded() const { return loaded_; }
        int serializerVersion() const { return serializerVersion_; }
        int poseMatcherVersion() const { return poseMatcherVersion_; }
        uint32_t signature() const { return signature_; }
        int samplesCount() const { return samplesCount_; }

    private:
        bool loaded_ = false;
        int serializerVersion_ = 0;
        int poseMatcherVersion_ = 0;
        uint32_t signature_ = 0;
        int samplesCount_ = 0;
    };
}
