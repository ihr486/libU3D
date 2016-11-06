#pragma once

namespace U3D
{

struct Quaternion4f;

static inline float inverse_quant(bool sign, uint32_t val, float iq)
{
    return sign ? -iq * val : iq * val;
}

struct Vector3f
{
    float x, y, z;
    Vector3f() : x(0), y(0), z(0) {}
    Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}
    float operator*(const Vector3f& v) const {
        return x * v.x + y * v.y + z * v.z;
    }
    Vector3f operator*(float a) const {
        return Vector3f(a * x, a * y, a * z);
    }
    Vector3f operator/(float a) const {
        return Vector3f(x / a, y / a, z / a);
    }
    Vector3f operator+(const Vector3f& v) const {
        return Vector3f(x + v.x, y + v.y, z + v.z);
    }
    Vector3f operator-(const Vector3f& v) const {
        return Vector3f(x - v.x, y - v.y, z - v.z);
    }
    Vector3f operator^(const Vector3f& v) const {
        return Vector3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    Vector3f normalize() const {
        float size = sqrtf(x * x + y * y + z * z);
        return (*this) / size;
    }
    Vector3f& operator=(const Quaternion4f& q);
    Vector3f& operator+=(const Vector3f& v) {
        x += v.x, y += v.y, z += v.z;
        return *this;
    }
    float size() const {
        return sqrtf(x * x + y * y + z * z);
    }
    static Vector3f dequantize(uint8_t signs, uint32_t x, uint32_t y, uint32_t z, float iq) {
        return Vector3f(inverse_quant(signs & 1, x, iq), inverse_quant(signs & 2, y, iq), inverse_quant(signs & 4, z, iq));
    }
};

static inline Vector3f slerp(const Vector3f& a, const Vector3f& b, float t)
{
    float omega = acosf(a * b);
    if(omega == 0) return a;
    return a * sinf((1 - t) * omega) / sinf(omega) + b * sinf(t * omega) / sinf(omega);
}

static inline std::ostream& operator<<(std::ostream& s, const Vector3f& v)
{
    return s << "[" << v.x << " " << v.y << " " << v.z << "]";
}

struct Vector2f
{
    float u, v;
    Vector2f() : u(0), v(0) {}
};

static inline std::ostream& operator<<(std::ostream& s, const Vector2f& v)
{
    return s << "[" << v.u << " " << v.v << "]";
}

struct Color3f
{
    float r, g, b;
    Color3f() : r(0), g(0), b(0) {}
    Color3f(float r, float g, float b) : r(r), g(g), b(b) {}
};

static inline std::ostream& operator<<(std::ostream& s, const Color3f& c)
{
    return s << "[" << c.r << " " << c.g << " " << c.b << "]";
}

struct Color4f
{
    float r, g, b, a;
    Color4f& operator+=(const Color4f& c) {
        r += c.r, g += c.g, b += c.b, a += c.a;
        return *this;
    }
    Color4f& operator/=(float val) {
        r /= val, g /= val, b /= val, a /= val;
        return *this;
    }
    Color4f operator+(const Color4f& c) const {
        return Color4f(r + c.r, g + c.g, b + c.b, a + c.a);
    }
    Color4f() : r(0), g(0), b(0), a(0) {}
    Color4f(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
    static Color4f dequantize(uint8_t signs, uint32_t r, uint32_t g, uint32_t b, uint32_t a, float iq) {
        return Color4f(inverse_quant(signs & 1, r, iq), inverse_quant(signs & 2, g, iq), inverse_quant(signs & 4, b, iq), inverse_quant(signs & 8, a, iq));
    }
};

static inline std::ostream& operator<<(std::ostream& s, const Color4f& c)
{
    return s << "[" << c.r << " " << c.g << " " << c.b << " " << c.a << "]";
}

struct Matrix4f
{
    float m[4][4];
    Matrix4f() {
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                m[i][j] = i == j ? 1.0f : 0.0f;
            }
        }
    }
    Matrix4f operator*(const Matrix4f& mat) const {
        Matrix4f ret;
        ret.m[0][0] = m[0][0] * mat.m[0][0] + m[1][0] * mat.m[0][1] + m[2][0] * mat.m[0][2] + m[3][0] * mat.m[0][3];
        ret.m[1][0] = m[0][0] * mat.m[1][0] + m[1][0] * mat.m[1][1] + m[2][0] * mat.m[1][2] + m[3][0] * mat.m[1][3];
        ret.m[2][0] = m[0][0] * mat.m[2][0] + m[1][0] * mat.m[2][1] + m[2][0] * mat.m[2][2] + m[3][0] * mat.m[2][3];
        ret.m[3][0] = m[0][0] * mat.m[3][0] + m[1][0] * mat.m[3][1] + m[2][0] * mat.m[3][2] + m[3][0] * mat.m[3][3];

        ret.m[0][1] = m[0][1] * mat.m[0][0] + m[1][1] * mat.m[0][1] + m[2][1] * mat.m[0][2] + m[3][1] * mat.m[0][3];
        ret.m[1][1] = m[0][1] * mat.m[1][0] + m[1][1] * mat.m[1][1] + m[2][1] * mat.m[1][2] + m[3][1] * mat.m[1][3];
        ret.m[2][1] = m[0][1] * mat.m[2][0] + m[1][1] * mat.m[2][1] + m[2][1] * mat.m[2][2] + m[3][1] * mat.m[2][3];
        ret.m[3][1] = m[0][1] * mat.m[3][0] + m[1][1] * mat.m[3][1] + m[2][1] * mat.m[3][2] + m[3][1] * mat.m[3][3];

        ret.m[0][2] = m[0][2] * mat.m[0][0] + m[1][2] * mat.m[0][1] + m[2][2] * mat.m[0][2] + m[3][2] * mat.m[0][3];
        ret.m[1][2] = m[0][2] * mat.m[1][0] + m[1][2] * mat.m[1][1] + m[2][2] * mat.m[1][2] + m[3][2] * mat.m[1][3];
        ret.m[2][2] = m[0][2] * mat.m[2][0] + m[1][2] * mat.m[2][1] + m[2][2] * mat.m[2][2] + m[3][2] * mat.m[2][3];
        ret.m[3][2] = m[0][2] * mat.m[3][0] + m[1][2] * mat.m[3][1] + m[2][2] * mat.m[3][2] + m[3][2] * mat.m[3][3];

        ret.m[0][3] = m[0][3] * mat.m[0][0] + m[1][3] * mat.m[0][1] + m[2][3] * mat.m[0][2] + m[3][3] * mat.m[0][3];
        ret.m[1][3] = m[0][3] * mat.m[1][0] + m[1][3] * mat.m[1][1] + m[2][3] * mat.m[1][2] + m[3][3] * mat.m[1][3];
        ret.m[2][3] = m[0][3] * mat.m[2][0] + m[1][3] * mat.m[2][1] + m[2][3] * mat.m[2][2] + m[3][3] * mat.m[2][3];
        ret.m[3][3] = m[0][3] * mat.m[3][0] + m[1][3] * mat.m[3][1] + m[2][3] * mat.m[3][2] + m[3][3] * mat.m[3][3];
        return ret;
    }
    Matrix4f& operator*=(const Matrix4f& mat) {
        float t[4][4];
        t[0][0] = m[0][0] * mat.m[0][0] + m[1][0] * mat.m[0][1] + m[2][0] * mat.m[0][2] + m[3][0] * mat.m[0][3];
        t[1][0] = m[0][0] * mat.m[1][0] + m[1][0] * mat.m[1][1] + m[2][0] * mat.m[1][2] + m[3][0] * mat.m[1][3];
        t[2][0] = m[0][0] * mat.m[2][0] + m[1][0] * mat.m[2][1] + m[2][0] * mat.m[2][2] + m[3][0] * mat.m[2][3];
        t[3][0] = m[0][0] * mat.m[3][0] + m[1][0] * mat.m[3][1] + m[2][0] * mat.m[3][2] + m[3][0] * mat.m[3][3];

        t[0][1] = m[0][1] * mat.m[0][0] + m[1][1] * mat.m[0][1] + m[2][1] * mat.m[0][2] + m[3][1] * mat.m[0][3];
        t[1][1] = m[0][1] * mat.m[1][0] + m[1][1] * mat.m[1][1] + m[2][1] * mat.m[1][2] + m[3][1] * mat.m[1][3];
        t[2][1] = m[0][1] * mat.m[2][0] + m[1][1] * mat.m[2][1] + m[2][1] * mat.m[2][2] + m[3][1] * mat.m[2][3];
        t[3][1] = m[0][1] * mat.m[3][0] + m[1][1] * mat.m[3][1] + m[2][1] * mat.m[3][2] + m[3][1] * mat.m[3][3];

        t[0][2] = m[0][2] * mat.m[0][0] + m[1][2] * mat.m[0][1] + m[2][2] * mat.m[0][2] + m[3][2] * mat.m[0][3];
        t[1][2] = m[0][2] * mat.m[1][0] + m[1][2] * mat.m[1][1] + m[2][2] * mat.m[1][2] + m[3][2] * mat.m[1][3];
        t[2][2] = m[0][2] * mat.m[2][0] + m[1][2] * mat.m[2][1] + m[2][2] * mat.m[2][2] + m[3][2] * mat.m[2][3];
        t[3][2] = m[0][2] * mat.m[3][0] + m[1][2] * mat.m[3][1] + m[2][2] * mat.m[3][2] + m[3][2] * mat.m[3][3];

        t[0][3] = m[0][3] * mat.m[0][0] + m[1][3] * mat.m[0][1] + m[2][3] * mat.m[0][2] + m[3][3] * mat.m[0][3];
        t[1][3] = m[0][3] * mat.m[1][0] + m[1][3] * mat.m[1][1] + m[2][3] * mat.m[1][2] + m[3][3] * mat.m[1][3];
        t[2][3] = m[0][3] * mat.m[2][0] + m[1][3] * mat.m[2][1] + m[2][3] * mat.m[2][2] + m[3][3] * mat.m[2][3];
        t[3][3] = m[0][3] * mat.m[3][0] + m[1][3] * mat.m[3][1] + m[2][3] * mat.m[3][2] + m[3][3] * mat.m[3][3];
        memcpy(m, t, sizeof(m));
        return *this;
    }
    Matrix4f inverse_as_view() const {
        Matrix4f ret;
        ret.m[0][0] = m[0][0];
        ret.m[1][0] = m[0][1];
        ret.m[2][0] = m[0][2];
        ret.m[3][0] = -m[0][0] * m[3][0] - m[0][1] * m[3][1] - m[0][2] * m[3][2];
        ret.m[0][1] = m[1][0];
        ret.m[1][1] = m[1][1];
        ret.m[2][1] = m[1][2];
        ret.m[3][1] = -m[1][0] * m[3][0] - m[1][1] * m[3][1] - m[1][2] * m[3][2];
        ret.m[0][2] = m[2][0];
        ret.m[1][2] = m[2][1];
        ret.m[2][2] = m[2][2];
        ret.m[3][2] = -m[2][0] * m[3][0] - m[2][1] * m[3][1] - m[2][2] * m[3][2];
        return ret;
    }
    Matrix4f create_normal_matrix() const {
        Matrix4f ret;
        float invdet = 1.0f / (m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] \
                  - m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1]);
        ret.m[0][0] = invdet * (m[1][1] * m[2][2] - m[2][1] * m[1][2]);
        ret.m[1][0] = invdet * (m[0][2] * m[2][1] - m[0][1] * m[2][2]);
        ret.m[2][0] = invdet * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
        ret.m[0][1] = invdet * (m[2][0] * m[1][2] - m[1][0] * m[2][2]);
        ret.m[1][1] = invdet * (m[0][0] * m[2][2] - m[2][0] * m[0][2]);
        ret.m[2][1] = invdet * (m[1][0] * m[0][2] - m[0][0] * m[1][2]);
        ret.m[0][2] = invdet * (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
        ret.m[1][2] = invdet * (m[2][0] * m[0][1] - m[0][0] * m[2][1]);
        ret.m[2][2] = invdet * (m[0][0] * m[1][1] - m[1][0] * m[0][1]);
        return ret;
    }
    Matrix4f inverse() const {
        Matrix4f ret;
        float invdet = 1.0f / (m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] \
                  - m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1]);
        ret.m[0][0] = invdet * (m[1][1] * m[2][2] - m[2][1] * m[1][2]);
        ret.m[0][1] = invdet * (m[0][2] * m[2][1] - m[0][1] * m[2][2]);
        ret.m[0][2] = invdet * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
        ret.m[1][0] = invdet * (m[2][0] * m[1][2] - m[1][0] * m[2][2]);
        ret.m[1][1] = invdet * (m[0][0] * m[2][2] - m[2][0] * m[0][2]);
        ret.m[1][2] = invdet * (m[1][0] * m[0][2] - m[0][0] * m[1][2]);
        ret.m[2][0] = invdet * (m[1][0] * m[2][1] - m[2][0] * m[1][1]);
        ret.m[2][1] = invdet * (m[2][0] * m[0][1] - m[0][0] * m[2][1]);
        ret.m[2][2] = invdet * (m[0][0] * m[1][1] - m[1][0] * m[0][1]);
        ret.m[3][0] = -ret.m[0][0] * m[3][0] - ret.m[1][0] * m[3][1] - ret.m[2][0] * m[3][2];
        ret.m[3][1] = -ret.m[0][1] * m[3][1] - ret.m[1][1] * m[3][1] - ret.m[2][1] * m[3][2];
        ret.m[3][2] = -ret.m[0][2] * m[3][2] - ret.m[1][2] * m[3][1] - ret.m[2][2] * m[3][2];
        return ret;
    }
    static void create_perspective_projection(Matrix4f& mat, float fovy, float aspect, float near, float far)
    {
        float f = 1.0f / tanf(fovy * 0.5f);
        mat.m[0][0] = f / aspect;
        mat.m[1][1] = f;
        mat.m[2][2] = (near + far) / (near - far);
        mat.m[3][2] = 2.0f * near * far / (near - far);
        mat.m[2][3] = -1.0f;
        mat.m[3][3] = 0.0f;
    }
    static void create_orthogonal_projection(Matrix4f& mat, float height, float aspect, float near, float far)
    {
        mat.m[0][0] = 2.0f / (height / aspect);
        mat.m[1][1] = 2.0f / height;
        mat.m[2][2] = 2.0f / (near - far);
        mat.m[3][2] = (near + far) / (near - far);
    }
    Vector3f operator*(const Vector3f& v) const {
        Vector3f ret;
        ret.x = m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0];
        ret.y = m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1];
        ret.z = m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2];
        return ret;
    }
    bool is_view() const {
        float epsilon = 1E-6f; //Dimensionless
        Vector3f xv(m[0][0], m[1][0], m[2][0]);
        if(xv.size() < 1 - epsilon || 1 + epsilon < xv.size()) return false;
        Vector3f yv(m[0][1], m[1][1], m[2][1]);
        if(yv.size() < 1 - epsilon || 1 + epsilon < yv.size()) return false;
        Vector3f zv(m[0][2], m[1][2], m[2][2]);
        if(zv.size() < 1 - epsilon || 1 + epsilon < zv.size()) return false;
        if(fabsf(xv * yv) > epsilon || fabsf(yv * zv) > epsilon || fabsf(zv * xv) > epsilon) return false;
        return true;
    }
    void identity() {
        for(int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                m[i][j] = (i == j) ? 1 : 0;
            }
        }
    }
};

static inline std::ostream& operator<<(std::ostream& os, const Matrix4f& mat) {
    os << '[';
    for(int y = 0; y < 4; y++) {
        os << '[';
        for(int x = 0; x < 4; x++) {
            os << mat.m[x][y];
            if(x != 3) os << ' ';
        }
        os << ']';
        if(y != 3) os << std::endl;
    }
    return os << ']';
}

struct Quaternion4f
{
    float w, x, y, z;
    Quaternion4f() : w(0), x(0), y(0), z(0) {}
    Quaternion4f operator*(const Quaternion4f& q) const {
        Quaternion4f ret;
        ret.w = w * q.w - x * q.x - y * q.y - z * q.z;
        ret.x = w * q.x + q.w * x + y * q.z - z * q.y;
        ret.y = w * q.y + q.w * y + z * q.x - x * q.z;
        ret.z = w * q.z + q.w * z + x * q.y - y * q.x;
        return ret;
    }
    Quaternion4f(const Vector3f& v) {
        w = 0, x = v.x, y = v.y, z = v.z;
    }
};

inline Vector3f& Vector3f::operator=(const Quaternion4f& q) {
    x = q.x, y = q.y, z = q.z;
    return *this;
}

struct TexCoord4f
{
    float u, v, s, t;
    TexCoord4f& operator+=(const TexCoord4f& c) {
        u += c.u, v += c.v, s += c.s, t += c.t;
        return *this;
    }
    TexCoord4f& operator/=(float val) {
        u /= val, v /= val, s /= val, t /= val;
        return *this;
    }
    TexCoord4f operator+(const TexCoord4f& c) const {
        return TexCoord4f(u + c.u, v + c.v, s + c.s, t + c.t);
    }
    TexCoord4f() : u(0), v(0), s(0), t(0) {}
    TexCoord4f(float u, float v, float s, float t) : u(u), v(v), s(s), t(t) {}
    static TexCoord4f dequantize(uint8_t signs, uint32_t u, uint32_t v, uint32_t s, uint32_t t, float iq) {
        return TexCoord4f(inverse_quant(signs & 1, u, iq), inverse_quant(signs & 2, v, iq), inverse_quant(signs & 4, s, iq), inverse_quant(signs & 8, t, iq));
    }
};

static inline std::ostream& operator<<(std::ostream& s, const TexCoord4f& c)
{
    return s << "[" << c.u << " " << c.v << " " << c.s << " " << c.t << "]";
}

}
