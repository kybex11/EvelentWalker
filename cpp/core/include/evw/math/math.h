// Lightweight math types replacing SharpDX.Mathematics.
// Memory layout matches SharpDX (column components stored sequentially) so that
// binary (de)serialization in DataReader/DataWriter stays byte-compatible.
#pragma once

#include <cmath>
#include <cstdint>

namespace evw::math
{
    struct Vector2
    {
        float X = 0.0f;
        float Y = 0.0f;

        Vector2() = default;
        Vector2(float x, float y) : X(x), Y(y) {}

        Vector2 operator+(const Vector2& o) const { return {X + o.X, Y + o.Y}; }
        Vector2 operator-(const Vector2& o) const { return {X - o.X, Y - o.Y}; }
        Vector2 operator*(float s) const { return {X * s, Y * s}; }
        bool operator==(const Vector2& o) const { return X == o.X && Y == o.Y; }
    };

    struct Vector3
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;

        Vector3() = default;
        Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}

        Vector3 operator+(const Vector3& o) const { return {X + o.X, Y + o.Y, Z + o.Z}; }
        Vector3 operator-(const Vector3& o) const { return {X - o.X, Y - o.Y, Z - o.Z}; }
        Vector3 operator*(float s) const { return {X * s, Y * s, Z * s}; }
        bool operator==(const Vector3& o) const { return X == o.X && Y == o.Y && Z == o.Z; }

        float Length() const { return std::sqrt(X * X + Y * Y + Z * Z); }
        float LengthSquared() const { return X * X + Y * Y + Z * Z; }
        static float Dot(const Vector3& a, const Vector3& b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }
        static Vector3 Cross(const Vector3& a, const Vector3& b)
        {
            return {a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X};
        }
    };

    struct Vector4
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float W = 0.0f;

        Vector4() = default;
        Vector4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

        bool operator==(const Vector4& o) const { return X == o.X && Y == o.Y && Z == o.Z && W == o.W; }
    };

    struct Quaternion
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float W = 1.0f;

        Quaternion() = default;
        Quaternion(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

        float Length() const { return std::sqrt(X * X + Y * Y + Z * Z + W * W); }
        bool operator==(const Quaternion& o) const { return X == o.X && Y == o.Y && Z == o.Z && W == o.W; }
    };

    // 4x4 matrix. Field naming mirrors SharpDX: Mrc where r=row, c=column.
    struct Matrix
    {
        float M11 = 1, M12 = 0, M13 = 0, M14 = 0;
        float M21 = 0, M22 = 1, M23 = 0, M24 = 0;
        float M31 = 0, M32 = 0, M33 = 1, M34 = 0;
        float M41 = 0, M42 = 0, M43 = 0, M44 = 1;

        static Matrix Identity() { return Matrix{}; }

        bool operator==(const Matrix& o) const
        {
            return M11 == o.M11 && M12 == o.M12 && M13 == o.M13 && M14 == o.M14 &&
                   M21 == o.M21 && M22 == o.M22 && M23 == o.M23 && M24 == o.M24 &&
                   M31 == o.M31 && M32 == o.M32 && M33 == o.M33 && M34 == o.M34 &&
                   M41 == o.M41 && M42 == o.M42 && M43 == o.M43 && M44 == o.M44;
        }
    };
}
