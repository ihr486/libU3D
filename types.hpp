#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

namespace U3D
{

template<typename T> struct Vector3
{
    T x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    T operator*(const Vector3<T>& v) {
        return x * v.x + y * v.y + z * v.z;
    }
    Vector3<T> operator*(T a) {
        return Vector3<T>{a * x, a * y, a * z};
    }
    Vector3<T> operator+(const Vector3<T>& v) {
        return Vector3<T>{x + v.x, y + v.y, z + v.z};
    }
};

template<typename T> static inline Vector3<T> slerp(const Vector3<T>& a, const Vector3<T>& b, T t)
{
    T omega = acos(a * b);
    if(omega == 0) return a;
    return sin(t * omega) / sin(omega) * a + sin((1 - t) * omega) / sin(omega) * b;
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
    Color4() : r(0), g(0), b(0), a(0) {}
};

template<typename T> struct Matrix4
{
    T m[4][4];
};

template<typename T> struct Quaternion4
{
    T w, x, y, z;
};

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
    TexCoord4() : u(0), v(0), s(0), t(0) {}
};

using Vector3f = Vector3<float>;
using Matrix4f = Matrix4<float>;
using Color3f = Color3<float>;
using Quaternion4f = Quaternion4<float>;
using Vector2f = Vector2<float>;
using Color4f = Color4<float>;
using TexCoord4f = TexCoord4<float>;

class Modifier
{
public:
    Modifier() {}
    virtual ~Modifier() {}
};

static inline float inverse_quant(bool sign, uint32_t val, float iq)
{
    return sign ? -iq * val : iq * val;
}

template<typename T> static inline void insert_unique(std::vector<T>& cont, T val)
{
    typename std::vector<T>::iterator i;
    for(i = cont.begin(); i != cont.end(); i++) {
        if(*i == val) return;
        else if(*i < val) {
            cont.insert(i, val);
            return;
        }
    }
    if(i == cont.end()) {
        cont.push_back(val);
    }
}

template<typename T> static inline void greater_unique_sort(std::vector<T>& cont)
{
    std::sort(cont.begin(), cont.end(), std::greater<T>());
    cont.erase(std::unique(cont.begin(), cont.end()), cont.end());
}

template<typename T> static inline void print_vector(const std::vector<T>& v, const std::string& name)
{
    std::cerr << name << " : ";
    for(auto i : v) {
        std::cerr << i << " ";
    }
    std::cerr << std::endl;
}

}
