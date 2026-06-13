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

        Vector3 toVector3() const { return {X, Y, Z}; }
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

        Quaternion normalized() const
        {
            float l = Length();
            if (l <= 0.0f) return {0, 0, 0, 1};
            float inv = 1.0f / l;
            return {X * inv, Y * inv, Z * inv, W * inv};
        }
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

        // Element access by [row][col], 0-based.
        float at(int r, int c) const { return (&M11)[r * 4 + c]; }
        float& at(int r, int c) { return (&M11)[r * 4 + c]; }
    };

    // Row-major matrix multiply: (a * b)[r][c] = sum_k a[r][k]*b[k][c].
    inline Matrix mul(const Matrix& a, const Matrix& b)
    {
        Matrix m;
        const float* A = &a.M11;
        const float* B = &b.M11;
        float* M = &m.M11;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
            {
                float s = 0;
                for (int k = 0; k < 4; ++k) s += A[r * 4 + k] * B[k * 4 + c];
                M[r * 4 + c] = s;
            }
        return m;
    }

    // Transforms a point (w=1) using a row-vector * matrix convention.
    inline Vector3 transformPoint(const Matrix& m, const Vector3& v)
    {
        const float* M = &m.M11;
        float x = v.X * M[0] + v.Y * M[4] + v.Z * M[8] + M[12];
        float y = v.X * M[1] + v.Y * M[5] + v.Z * M[9] + M[13];
        float z = v.X * M[2] + v.Y * M[6] + v.Z * M[10] + M[14];
        return {x, y, z};
    }

    inline Matrix translation(float x, float y, float z)
    {
        Matrix m; // identity
        m.M41 = x; m.M42 = y; m.M43 = z;
        return m;
    }

    inline Matrix scaling(float x, float y, float z)
    {
        Matrix m;
        m.M11 = x; m.M22 = y; m.M33 = z;
        return m;
    }

    // Right-handed perspective projection (fovY in radians).
    inline Matrix perspectiveFovRH(float fovY, float aspect, float zn, float zf)
    {
        float h = 1.0f / std::tan(fovY * 0.5f);
        float w = h / aspect;
        Matrix m{};
        float* M = &m.M11;
        for (int i = 0; i < 16; ++i) M[i] = 0;
        M[0] = w;
        M[5] = h;
        M[10] = zf / (zn - zf);
        M[11] = -1.0f;
        M[14] = (zn * zf) / (zn - zf);
        return m;
    }

    // Right-handed look-at view matrix.
    inline Matrix lookAtRH(const Vector3& eye, const Vector3& target, const Vector3& up)
    {
        Vector3 zaxis = eye - target; // RH: camera looks down -z
        float zl = zaxis.Length(); if (zl > 0) zaxis = zaxis * (1.0f / zl);
        Vector3 xaxis = Vector3::Cross(up, zaxis);
        float xl = xaxis.Length(); if (xl > 0) xaxis = xaxis * (1.0f / xl);
        Vector3 yaxis = Vector3::Cross(zaxis, xaxis);

        Matrix m{};
        float* M = &m.M11;
        M[0] = xaxis.X; M[1] = yaxis.X; M[2] = zaxis.X; M[3] = 0;
        M[4] = xaxis.Y; M[5] = yaxis.Y; M[6] = zaxis.Y; M[7] = 0;
        M[8] = xaxis.Z; M[9] = yaxis.Z; M[10] = zaxis.Z; M[11] = 0;
        M[12] = -Vector3::Dot(xaxis, eye);
        M[13] = -Vector3::Dot(yaxis, eye);
        M[14] = -Vector3::Dot(zaxis, eye);
        M[15] = 1;
        return m;
    }

    // Converts a (normalized) quaternion to a rotation matrix (row-major).
    inline Matrix quaternionToMatrix(const Quaternion& q)
    {
        float x = q.X, y = q.Y, z = q.Z, w = q.W;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;
        Matrix m;
        m.M11 = 1 - 2 * (yy + zz); m.M12 = 2 * (xy + wz);     m.M13 = 2 * (xz - wy);     m.M14 = 0;
        m.M21 = 2 * (xy - wz);     m.M22 = 1 - 2 * (xx + zz); m.M23 = 2 * (yz + wx);     m.M24 = 0;
        m.M31 = 2 * (xz + wy);     m.M32 = 2 * (yz - wx);     m.M33 = 1 - 2 * (xx + yy); m.M34 = 0;
        m.M41 = 0; m.M42 = 0; m.M43 = 0; m.M44 = 1;
        return m;
    }
}
