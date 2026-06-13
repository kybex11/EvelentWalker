// Port of EvelentWalker.Core/GameFiles/Utils/Data.cs
// The C# version reads/writes over a System.IO.Stream. Here we operate over an
// in-memory byte buffer, which covers all current usage (resources are loaded
// fully into memory before parsing).
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "evw/math/math.h"

namespace evw::gamefiles
{
    enum class Endianess
    {
        LittleEndian,
        BigEndian
    };

    enum class DataType
    {
        Byte = 0,
        Int16 = 1,
        Int32 = 2,
        Int64 = 3,
        Uint16 = 4,
        Uint32 = 5,
        Uint64 = 6,
        Float = 7,
        Double = 8,
        String = 9,
    };

    class DataReader
    {
    public:
        explicit DataReader(std::vector<uint8_t> data, Endianess endianess = Endianess::LittleEndian);
        DataReader(const uint8_t* data, size_t length, Endianess endianess = Endianess::LittleEndian);

        Endianess endianess() const { return endianess_; }
        void setEndianess(Endianess e) { endianess_ = e; }

        int64_t length() const { return static_cast<int64_t>(buffer_.size()); }
        int64_t position() const { return position_; }
        void setPosition(int64_t pos) { position_ = pos; }

        uint8_t ReadByte();
        std::vector<uint8_t> ReadBytes(int count);
        int16_t ReadInt16();
        int32_t ReadInt32();
        int64_t ReadInt64();
        uint16_t ReadUInt16();
        uint32_t ReadUInt32();
        uint64_t ReadUInt64();
        float ReadSingle();
        double ReadDouble();
        std::string ReadString(); // null-terminated UTF-8

        math::Vector3 ReadVector3();
        math::Vector4 ReadVector4();
        math::Matrix ReadMatrix();

        static uint32_t SizeOf(DataType type);

    private:
        // Reads `count` bytes; applies endian reversal unless ignoreEndianess.
        void readRaw(uint8_t* out, int count, bool ignoreEndianess = false);

        std::vector<uint8_t> buffer_;
        int64_t position_ = 0;
        Endianess endianess_;
    };

    class DataWriter
    {
    public:
        explicit DataWriter(Endianess endianess = Endianess::LittleEndian);

        Endianess endianess() const { return endianess_; }
        void setEndianess(Endianess e) { endianess_ = e; }

        int64_t length() const { return static_cast<int64_t>(buffer_.size()); }
        int64_t position() const { return position_; }
        void setPosition(int64_t pos) { position_ = pos; }

        const std::vector<uint8_t>& buffer() const { return buffer_; }

        void Write(uint8_t value);
        void Write(const std::vector<uint8_t>& value);
        void Write(int16_t value);
        void Write(int32_t value);
        void Write(int64_t value);
        void Write(uint16_t value);
        void Write(uint32_t value);
        void Write(uint64_t value);
        void Write(float value);
        void Write(double value);
        void WriteString(const std::string& value); // null-terminated

        void Write(const math::Vector3& value);
        void Write(const math::Vector4& value);
        void Write(const math::Matrix& value);

    private:
        void writeRaw(const uint8_t* value, int count, bool ignoreEndianess = false);

        std::vector<uint8_t> buffer_;
        int64_t position_ = 0;
        Endianess endianess_;
    };
}
