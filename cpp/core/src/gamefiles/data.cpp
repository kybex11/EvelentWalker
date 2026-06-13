#include "evw/gamefiles/data.h"

#include <algorithm>
#include <cstring>

namespace evw::gamefiles
{
    // ---- DataReader ----

    DataReader::DataReader(std::vector<uint8_t> data, Endianess endianess)
        : buffer_(std::move(data)), endianess_(endianess)
    {
    }

    DataReader::DataReader(const uint8_t* data, size_t length, Endianess endianess)
        : buffer_(data, data + length), endianess_(endianess)
    {
    }

    void DataReader::readRaw(uint8_t* out, int count, bool ignoreEndianess)
    {
        if (count < 0)
            throw std::out_of_range("DataReader: negative read count");
        if (position_ + count > static_cast<int64_t>(buffer_.size()))
            throw std::out_of_range("DataReader: read past end of buffer");

        std::memcpy(out, buffer_.data() + position_, static_cast<size_t>(count));
        position_ += count;

        if (!ignoreEndianess && endianess_ == Endianess::BigEndian)
            std::reverse(out, out + count);
    }

    uint8_t DataReader::ReadByte()
    {
        uint8_t v;
        readRaw(&v, 1);
        return v;
    }

    std::vector<uint8_t> DataReader::ReadBytes(int count)
    {
        std::vector<uint8_t> out(static_cast<size_t>(count));
        if (count > 0) readRaw(out.data(), count, true);
        return out;
    }

    int16_t DataReader::ReadInt16() { int16_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 2); return v; }
    int32_t DataReader::ReadInt32() { int32_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 4); return v; }
    int64_t DataReader::ReadInt64() { int64_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 8); return v; }
    uint16_t DataReader::ReadUInt16() { uint16_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 2); return v; }
    uint32_t DataReader::ReadUInt32() { uint32_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 4); return v; }
    uint64_t DataReader::ReadUInt64() { uint64_t v; readRaw(reinterpret_cast<uint8_t*>(&v), 8); return v; }
    float DataReader::ReadSingle() { float v; readRaw(reinterpret_cast<uint8_t*>(&v), 4); return v; }
    double DataReader::ReadDouble() { double v; readRaw(reinterpret_cast<uint8_t*>(&v), 8); return v; }

    std::string DataReader::ReadString()
    {
        std::string result;
        uint8_t c = ReadByte();
        while (c != 0)
        {
            result.push_back(static_cast<char>(c));
            c = ReadByte();
        }
        return result;
    }

    math::Vector3 DataReader::ReadVector3()
    {
        math::Vector3 v;
        v.X = ReadSingle();
        v.Y = ReadSingle();
        v.Z = ReadSingle();
        return v;
    }

    math::Vector4 DataReader::ReadVector4()
    {
        math::Vector4 v;
        v.X = ReadSingle();
        v.Y = ReadSingle();
        v.Z = ReadSingle();
        v.W = ReadSingle();
        return v;
    }

    math::Matrix DataReader::ReadMatrix()
    {
        // Matches C# read order (column-major sequence).
        math::Matrix m;
        m.M11 = ReadSingle(); m.M21 = ReadSingle(); m.M31 = ReadSingle(); m.M41 = ReadSingle();
        m.M12 = ReadSingle(); m.M22 = ReadSingle(); m.M32 = ReadSingle(); m.M42 = ReadSingle();
        m.M13 = ReadSingle(); m.M23 = ReadSingle(); m.M33 = ReadSingle(); m.M43 = ReadSingle();
        m.M14 = ReadSingle(); m.M24 = ReadSingle(); m.M34 = ReadSingle(); m.M44 = ReadSingle();
        return m;
    }

    uint32_t DataReader::SizeOf(DataType type)
    {
        switch (type)
        {
        case DataType::Byte: return 1;
        case DataType::Int16: return 2;
        case DataType::Int32: return 4;
        case DataType::Int64: return 8;
        case DataType::Uint16: return 2;
        case DataType::Uint32: return 4;
        case DataType::Uint64: return 8;
        case DataType::Float: return 4;
        case DataType::Double: return 8;
        case DataType::String: return 0;
        default: return 1;
        }
    }

    // ---- DataWriter ----

    DataWriter::DataWriter(Endianess endianess) : endianess_(endianess) {}

    void DataWriter::writeRaw(const uint8_t* value, int count, bool ignoreEndianess)
    {
        std::vector<uint8_t> tmp(value, value + count);
        if (!ignoreEndianess && endianess_ == Endianess::BigEndian)
            std::reverse(tmp.begin(), tmp.end());

        // Honor the current position (allows overwriting), growing as needed.
        if (position_ + count > static_cast<int64_t>(buffer_.size()))
            buffer_.resize(static_cast<size_t>(position_ + count));
        std::memcpy(buffer_.data() + position_, tmp.data(), static_cast<size_t>(count));
        position_ += count;
    }

    void DataWriter::Write(uint8_t value) { writeRaw(&value, 1); }

    void DataWriter::Write(const std::vector<uint8_t>& value)
    {
        if (!value.empty()) writeRaw(value.data(), static_cast<int>(value.size()), true);
    }

    void DataWriter::Write(int16_t value) { writeRaw(reinterpret_cast<uint8_t*>(&value), 2); }
    void DataWriter::Write(int32_t value) { writeRaw(reinterpret_cast<uint8_t*>(&value), 4); }
    void DataWriter::Write(int64_t value) { writeRaw(reinterpret_cast<uint8_t*>(&value), 8); }
    void DataWriter::Write(uint16_t value) { writeRaw(reinterpret_cast<uint8_t*>(&value), 2); }
    void DataWriter::Write(uint32_t value) { writeRaw(reinterpret_cast<uint8_t*>(&value), 4); }
    void DataWriter::Write(uint64_t value) { writeRaw(reinterpret_cast<uint8_t*>(&value), 8); }
    void DataWriter::Write(float value) { writeRaw(reinterpret_cast<uint8_t*>(&value), 4); }
    void DataWriter::Write(double value) { writeRaw(reinterpret_cast<uint8_t*>(&value), 8); }

    void DataWriter::WriteString(const std::string& value)
    {
        for (char c : value) Write(static_cast<uint8_t>(c));
        Write(static_cast<uint8_t>(0));
    }

    void DataWriter::Write(const math::Vector3& value)
    {
        Write(value.X); Write(value.Y); Write(value.Z);
    }

    void DataWriter::Write(const math::Vector4& value)
    {
        Write(value.X); Write(value.Y); Write(value.Z); Write(value.W);
    }

    void DataWriter::Write(const math::Matrix& value)
    {
        Write(value.M11); Write(value.M21); Write(value.M31); Write(value.M41);
        Write(value.M12); Write(value.M22); Write(value.M32); Write(value.M42);
        Write(value.M13); Write(value.M23); Write(value.M33); Write(value.M43);
        Write(value.M14); Write(value.M24); Write(value.M34); Write(value.M44);
    }
}
