#pragma once

#include <cmath>
#include <stdint.h>

namespace U3D
{

template<typename T> struct Quaternion4;

static inline float inverse_quant(bool sign, uint32_t val, float iq)
{
    return sign ? -iq * val : iq * val;
}

template<typename T> struct Vector3
{
    T x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(T x, T y, T z) : x(x), y(y), z(z) {}
    T operator*(const Vector3<T>& v) const {
        return x * v.x + y * v.y + z * v.z;
    }
    Vector3<T> operator*(T a) const {
        return Vector3<T>(a * x, a * y, a * z);
    }
    Vector3<T> operator/(T a) const {
        return Vector3<T>(x / a, y / a, z / a);
    }
    Vector3<T> operator+(const Vector3<T>& v) const {
        return Vector3<T>(x + v.x, y + v.y, z + v.z);
    }
    Vector3<T> operator-(const Vector3<T>& v) const {
        return Vector3<T>(x - v.x, y - v.y, z - v.z);
    }
    Vector3<T> operator^(const Vector3<T>& v) const {
        return Vector3<T>(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    Vector3<T> normalize() const {
        T size = sqrt(x * x + y * y + z * z);
        return (*this) / size;
    }
    Vector3<T>& operator=(const Quaternion4<T>& q);
    Vector3<T>& operator+=(const Vector3<T>& v) {
        x += v.x, y += v.y, z += v.z;
        return *this;
    }
    static Vector3<T> dequantize(uint8_t signs, uint32_t x, uint32_t y, uint32_t z, T iq) {
        return Vector3<T>(inverse_quant(signs & 1, x, iq), inverse_quant(signs & 2, y, iq), inverse_quant(signs & 4, z, iq));
    }
};

template<typename T> static inline Vector3<T> slerp(const Vector3<T>& a, const Vector3<T>& b, T t)
{
    T omega = std::acos(a * b);
    if(omega == 0) return a;
    return a * std::sin((1 - t) * omega) / std::sin(omega) + b * std::sin(t * omega) / std::sin(omega);
}

template<typename T> struct Vector2
{
    T u, v;
    Vector2() : u(0), v(0) {}
};

template<typename T> struct Color3
{
    T r, g, b;
    Color3() : r(0), g(0), b(0) {}
};

template<typename T> struct Color4
{
    T r, g, b, a;
    Color4<T>& operator+=(const Color4<T>& c) {
        r += c.r, g += c.g, b += c.b, a += c.a;
        return *this;
    }
    Color4<T>& operator/=(T val) {
        r /= val, g /= val, b /= val, a /= val;
        return *this;
    }
    Color4<T> operator+(const Color4<T>& c) {
        return Color4<T>(r + c.r, g + c.g, b + c.b, a + c.a);
    }
    Color4() : r(0), g(0), b(0), a(0) {}
    Color4(T r, T g, T b, T a) : r(r), g(g), b(b), a(a) {}
    static Color4<T> dequantize(uint8_t signs, uint32_t r, uint32_t g, uint32_t b, uint32_t a, float iq) {
        return Color4<T>(inverse_quant(signs & 1, r, iq), inverse_quant(signs & 2, g, iq), inverse_quant(signs & 4, b, iq), inverse_quant(signs & 8, a, iq));
    }
};

template<typename T> struct Matrix4
{
    T m[4][4];
};

template<typename T> struct Quaternion4
{
    T w, x, y, z;
    Quaternion4() : w(0), x(0), y(0), z(0) {}
    Quaternion4<T> operator*(const Quaternion4<T>& q) {
        Quaternion4<T> ret;
        ret.w = w * q.w - x * q.x - y * q.y - z * q.z;
        ret.x = w * q.x + q.w * x + y * q.z - z * q.y;
        ret.y = w * q.y + q.w * y + z * q.x - x * q.z;
        ret.z = w * q.z + q.w * z + x * q.y - y * q.x;
        return ret;
    }
    Quaternion4(const Vector3<T>& v) {
        w = 0, x = v.x, y = v.y, z = v.z;
    }
};

template<typename T> Vector3<T>& Vector3<T>::operator=(const Quaternion4<T>& q) {
    x = q.x, y = q.y, z = q.z;
    return *this;
}

template<typename T> struct TexCoord4
{
    T u, v, s, t;
    TexCoord4<T>& operator+=(const TexCoord4<T>& c) {
        u += c.u, v += c.v, s += c.s, t += c.t;
        return *this;
    }
    TexCoord4<T>& operator/=(T val) {
        u /= val, v /= val, s /= val, t /= val;
        return *this;
    }
    TexCoord4<T> operator+(const TexCoord4<T>& c) {
        return TexCoord4<T>(u + c.u, v + c.v, s + c.s, t + c.t);
    }
    TexCoord4() : u(0), v(0), s(0), t(0) {}
    TexCoord4(T u, T v, T s, T t) : u(u), v(v), s(s), t(t) {}
    static TexCoord4<T> dequantize(uint8_t signs, uint32_t u, uint32_t v, uint32_t s, uint32_t t, float iq) {
        return TexCoord4<T>(inverse_quant(signs & 1, u, iq), inverse_quant(signs & 2, v, iq), inverse_quant(signs & 4, s, iq), inverse_quant(signs & 8, t, iq));
    }
};

typedef Vector3<float> Vector3f;
typedef Matrix4<float> Matrix4f;
typedef Color3<float> Color3f;
typedef Quaternion4<float> Quaternion4f;
typedef Vector2<float> Vector2f;
typedef Color4<float> Color4f;
typedef TexCoord4<float> TexCoord4f;

class Modifier
{
public:
    Modifier() {}
    virtual ~Modifier() {}
};

}
