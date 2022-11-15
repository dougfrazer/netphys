#pragma once

#include "quaternion.h"
#include "lib.h"

//******************************************************************************
// Vector - Four-Dimensional Vector
//******************************************************************************
class vector4
{
public:
    float x;
    float y;
    float z;
    float w;

public:
    vector4()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        w = 0.0f;
    }
    vector4(const float _x, const float _y, const float _z, float _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }

    vector4 operator-() const
    {
        vector4 v;
        v.x = -x;
        v.y = -y;
        v.z = -z;
        v.w = -w;
        return v;
    }
    vector4 operator-(const vector4& B) const
    {
        vector4 v;
        v.x = x - B.x;
        v.y = y - B.y;
        v.z = z - B.z;
        v.w = w - B.w;
        return v;
    }
    vector4 operator+(const vector4& B) const
    {
        vector4 v;
        v.x = x + B.x;
        v.y = y + B.y;
        v.z = z + B.z;
        v.w = z + B.w;
        return v;
    }
    vector4 operator*(const float s)   const
    {
        vector4 v;
        v.x = x * s;
        v.y = y * s;
        v.z = z * s;
        v.w = w * s;
        return v;
    }

    float    dot(const vector4& B) const
    {
        return x * B.x + y * B.y + z * B.z + w * B.w;
    }
    vector4 proj(const vector4& B) const
    {
        float B_mag = B.magnitude();
        vector4 v = B * (dot(B) / (B_mag * B_mag));
        return v;
    }
    float    magnitude(void)        const
    {
        return sqrt(x * x + y * y + z * z + w * w);
    }
    vector4  normalize(void)        const
    {
        vector4 v;
        float m = magnitude();
        v.x = x / m;
        v.y = y / m;
        v.z = z / m;
        v.w = w / m;
        return v;
    }
};


//******************************************************************************
// Vector - Three-Dimensional Vector
//******************************************************************************
class vector3
{
public:
    float x;
    float y;
    float z;

public:
    vector3()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }
    vector3(const float v)
    {
        x = v;
        y = v;
        z = v;
    }
    vector3(const vector4& v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
    }
    vector3(const float _x, const float _y, const float _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    vector3 operator-() const
    {
        vector3 v;
        v.x = -x;
        v.y = -y;
        v.z = -z;
        return v;
    }
    vector3 operator-(const vector3 B) const
    {
        vector3 v;
        v.x = x - B.x;
        v.y = y - B.y;
        v.z = z - B.z;
        return v;
    }
    vector3 operator+(const vector3 B) const
    {
        vector3 v;
        v.x = x + B.x;
        v.y = y + B.y;
        v.z = z + B.z;
        return v;
    }
    vector3 operator*(const float s)   const
    {
        vector3 v;
        v.x = x * s;
        v.y = y * s;
        v.z = z * s;
        return v;
    }
    vector3 operator/(const float s)   const
    {
        vector3 v;
        v.x = x / s;
        v.y = y / s;
        v.z = z / s;
        return v;
    }
    bool operator==(const vector3& b) const
    {
        return FloatEquals(x,b.x) && FloatEquals(y,b.y) && FloatEquals(z,b.z);
    }
    float    dot(const vector3& B) const
    {
        return x * B.x + y * B.y + z * B.z;
    }
    vector3  cross(const vector3& B) const
    {
        vector3 v;
        v.x = y * B.z - z * B.y;
        v.y = z * B.x - x * B.z;
        v.z = x * B.y - y * B.x;
        return v;
    }
    vector3  proj(const vector3& B) const
    {
        float B_mag = B.magnitude();
        vector3 v = B * (dot(B) / (B_mag * B_mag));
        return v;
    }
    float    magnitude_sq(void)     const
    {
        return x*x + y*y + z*z;
    }
    float    magnitude(void)        const
    {
        return sqrt(magnitude_sq());
    }
    vector3  normalize(void)        const
    {
        vector3 v;
        float m = magnitude();
        v.x = x / m;
        v.y = y / m;
        v.z = z / m;
        return v;
    }

    bool IsNone() const { return FloatEquals(x, 0.0f) && FloatEquals(y, 0.0f) && FloatEquals(z, 0.0f); }

    // https://en.wikipedia.org/wiki/Triple_product
    // geometrically represents the signed volume of the parallelpiped defined by these three vectors
	static float triple_product(const vector3& a, const vector3& b, const vector3& c)
	{
		return a.dot(b.cross(c));
	}
    static float box(const vector3& a, const vector3& b, const vector3& c)
    {
        return triple_product(a,b,c);
    }

    static float cross(const vector3& a, const vector3& b)
    {
        return a.cross(b).magnitude();
    }
//    vector3  rotate(vector3 axis, float degrees) const
//    {
//        quaternion q(axis, degrees);
//        quaternion p(x, y, z, 0.0);
//        quaternion p_prime = q * p * q.inverse();
//        return vector3(p_prime.x, p_prime.y, p_prime.z);
//    }
};

namespace Coordinates
{
    static vector3 GetUp()    { return vector3( 0.0f, 1.0f, 0.0f); }
    static vector3 GetDown()  { return vector3( 0.0f,-1.0f, 0.0f); }
    static vector3 GetLeft()  { return vector3( 0.0f, 0.0f,-1.0f); }
    static vector3 GetRight() { return vector3( 0.0f, 0.0f, 1.0f); }
    static vector3 GetIn()    { return vector3( 1.0f, 0.0f, 0.0f); }
    static vector3 GetOut()   { return vector3(-1.0f, 0.0f, 0.0f); }
}

