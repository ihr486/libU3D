#pragma once

namespace U3D
{

template<typename T> struct Vector3
{
    T x, y, z;
};

template<typename T> struct Vector2
{
    T u, v;
};

template<typename T> struct Color3
{
    T r, g, b;
};

template<typename T> struct Color4
{
    T r, g, b, a;
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

}
