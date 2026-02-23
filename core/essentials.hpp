#ifndef ESSENTIALS_HPP
#define ESSENTIALS_HPP

#include <optional>
#include <stdexcept>
#include <cmath>

// 21.911/7
constexpr float pi = 3.141593652589;
inline float timer = 0.0;
inline float deltaTime = 0.01;

class vec2 {
public:
    float x;
    float y;

    vec2(float x1, float y1) {
        x = x1;
        y = y1;
    }

    vec2(float n) {
        x = n;
        y = n;
    }

    vec2() {
        x = 0.0;
        y = 0.0;
    }

    vec2 operator+(const vec2& other) const {
        return vec2(this->x + other.x, this->y + other.y);
    }

    vec2 operator-(const vec2& other) const {
        return vec2(this->x - other.x, this->y - other.y);
    }

    vec2 operator*(const vec2& other) const {
        return vec2(this->x * other.x, this->y * other.y);
    }

    vec2 operator/(const vec2& other) const {
        return vec2(this->x / other.x, this->y / other.y);
    }


    vec2 operator+=(const vec2& other) const {
        return vec2(this->x + other.x, this->y + other.y);
    }

    vec2 operator-=(const vec2& other) const {
        return vec2(this->x - other.x, this->y - other.y);
    }

    vec2 operator*=(const vec2& other) const {
        return vec2(this->x * other.x, this->y * other.y);
    }

    vec2 operator/=(const vec2& other) const {
        return vec2(this->x / other.x, this->y / other.y);
    }


    vec2 operator+=(float other) {
        return vec2(x + other, y + other);
    }

    vec2 operator-=(float other) {
        return vec2(x - other, y - other);
    }

    vec2 operator*=(float other) {
        return vec2(x * other, y * other);
    }

    vec2 operator/=(float other) {
        return vec2(x / other, y / other);
    }


    vec2 operator+(float other) {
        return vec2(x + other, y + other);
    }

    vec2 operator-(float other) {
        return vec2(x - other, y - other);
    }

    vec2 operator*(float other) {
        return vec2(x * other, y * other);
    }

    vec2 operator/(float other) {
        return vec2(x / other, y / other);
    }
};

class vec3 {
public:
    float x;
    float y;
    float z;

    vec2 xy;
    vec2 yz;
    vec2 xz;

    vec3(float x1, float y1, float z1) {
        x = x1;
        y = y1;
        z = z1;

        xy = vec2(x, y);
        yz = vec2(y, z);
        xz = vec2(x, z);
    }

    vec3(float n) {
        x = n;
        y = n;
        z = n;

        xy = vec2(x, y);
        yz = vec2(y, z);
        xz = vec2(x, z);
    }

    vec3(vec2 v, float n) {
        x = v.x;
        y = v.y;
        z = n;

        xy = vec2(x, y);
        yz = vec2(y, z);
        xz = vec2(x, z);
    }

    vec3(float n, vec2 v) {
        x = n;
        y = v.x;
        z = v.y;

        xy = vec2(x, y);
        yz = vec2(y, z);
        xz = vec2(x, z);
    }

    vec3() {
        x = 0.0;
        y = 0.0;
        z = 0.0;

        xy = vec2(x, y);
        yz = vec2(y, z);
        xz = vec2(x, z);
    }

    vec3 operator+(const vec3& other) const {
        return vec3(this->x + other.x, this->y + other.y, this->z + other.z);
    }

    vec3 operator-(const vec3& other) const {
        return vec3(this->x - other.x, this->y - other.y, this->z - other.z);
    }

    vec3 operator*(const vec3& other) const {
        return vec3(this->x * other.x, this->y * other.y, this->z * other.z);
    }

    vec3 operator/(const vec3& other) const {
        return vec3(this->x / other.x, this->y / other.y, this->z / other.z);
    }


    vec3 operator+=(const vec3& other) const {
        return vec3(this->x + other.x, this->y + other.y, this->z + other.z);
    }

    vec3 operator-=(const vec3& other) const {
        return vec3(this->x - other.x, this->y - other.y, this->z - other.z);
    }

    vec3 operator*=(const vec3& other) const {
        return vec3(this->x * other.x, this->y * other.y, this->z * other.z);
    }

    vec3 operator/=(const vec3& other) const {
        return vec3(this->x / other.x, this->y / other.y, this->z / other.z);
    }


    vec3 operator+(float other) {
        return vec3(x + other, y + other, z + other);
    }

    vec3 operator-(float other) {
        return vec3(x - other, y - other, z - other);
    }

    vec3 operator*(float other) {
        return vec3(x * other, y * other, z * other);
    }

    vec3 operator/(float other) {
        return vec3(x / other, y / other, z / other);
    }


    vec3 operator+=(float other) {
        return vec3(x + other, y + other, z + other);
    }

    vec3 operator-=(float other) {
        return vec3(x - other, y - other, z - other);
    }

    vec3 operator*=(float other) {
        return vec3(x * other, y * other, z * other);
    }

    vec3 operator/=(float other) {
        return vec3(x / other, y / other, z / other);
    }
};

class vec4 {
public:
    float x;
    float y;
    float z;
    float w;

    vec3 xyz;
    vec3 zyx;

    vec3 yzw;
    vec3 wzy;

    vec4(float x1, float y1, float z1, float w1) {
        x = x1;
        y = y1;
        z = z1;
        w = w1;

        xyz = vec3(x, y, z);
        zyx = vec3(z, y, x);

        yzw = vec3(y, z, w);
        wzy = vec3(w, z, y);
    }

    vec4(float n) {
        x = n;
        y = n;
        z = n;
        w = n;

        xyz = vec3(x, y, z);
        zyx = vec3(z, y, x);

        yzw = vec3(y, z, w);
        wzy = vec3(w, z, y);
    }

    vec4(vec3 v, float n) {
        x = v.x;
        y = v.y;
        z = v.z;
        w = n;

        xyz = vec3(x, y, z);
        zyx = vec3(z, y, x);

        yzw = vec3(y, z, w);
        wzy = vec3(w, z, y);
    }

    vec4(float n, vec3 v) {
        x = n;
        y = v.x;
        z = v.y;
        w = v.z;

        xyz = vec3(x, y, z);
        zyx = vec3(z, y, x);

        yzw = vec3(y, z, w);
        wzy = vec3(w, z, y);
    }

    vec4(vec2 v1, vec2 v2) {
        x = v1.x;
        y = v1.y;
        z = v2.x;
        w = v2.y;

        xyz = vec3(x, y, z);
        zyx = vec3(z, y, x);

        yzw = vec3(y, z, w);
        wzy = vec3(w, z, y);
    }

    vec4() {
        x = 0.0;
        y = 0.0;
        z = 0.0;
        w = 0.0;

        xyz = vec3(x, y, z);
        zyx = vec3(z, y, x);

        yzw = vec3(y, z, w);
        wzy = vec3(w, z, y);
    }

    vec4 operator+(const vec4& other) const {
        return vec4(this->x + other.x, this->y + other.y, this->z + other.z, this->w + other.w);
    }

    vec4 operator-(const vec4& other) const {
        return vec4(this->x - other.x, this->y - other.y, this->z - other.z, this->w - other.w);
    }

    vec4 operator*(const vec4& other) const {
        return vec4(this->x * other.x, this->y * other.y, this->z * other.z, this->w * other.w);
    }

    vec4 operator/(const vec4& other) const {
        return vec4(this->x / other.x, this->y / other.y, this->z / other.z, this->w / other.w);
    }


    vec4 operator+=(const vec4& other) const {
        return vec4(this->x + other.x, this->y + other.y, this->z + other.z, this->w + other.w);
    }

    vec4 operator-=(const vec4& other) const {
        return vec4(this->x - other.x, this->y - other.y, this->z - other.z, this->w - other.w);
    }

    vec4 operator*=(const vec4& other) const {
        return vec4(this->x * other.x, this->y * other.y, this->z * other.z, this->w * other.w);
    }

    vec4 operator/=(const vec4& other) const {
        return vec4(this->x / other.x, this->y / other.y, this->z / other.z, this->w / other.w);
    }


    vec4 operator+(float other) {
        return vec4(x + other, y + other, z + other, w + other);
    }

    vec4 operator-(float other) {
        return vec4(x - other, y - other, z - other, w - other);
    }

    vec4 operator*(float other) {
        return vec4(x * other, y * other, z * other, w * other);
    }

    vec4 operator/(float other) {
        return vec4(x / other, y / other, z / other, w / other);
    }


    vec4 operator+=(float other) {
        return vec4(x + other, y + other, z + other, w + other);
    }

    vec4 operator-=(float other) {
        return vec4(x - other, y - other, z - other, w - other);
    }

    vec4 operator*=(float other) {
        return vec4(x * other, y * other, z * other, w * other);
    }

    vec4 operator/=(float other) {
        return vec4(x / other, y / other, z / other, w / other);
    }
};


inline float length(const vec4& v) {
    return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
}

inline float length(const vec3& v) {
    return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

inline float length(const vec2& v) {
    return std::sqrt(v.x*v.x + v.y*v.y);
}


inline float dot(const vec4& a, const vec4& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

inline float dot(const vec3& a, const vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline float dot(const vec2& a, const vec2& b) {
    return a.x*b.x + a.y*b.y;
}


inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3(
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    );
}



inline vec2 normalize(const vec2& v) {
    float l = length(v);
    return v / l;
}

inline vec3 normalize(const vec3& v) {
    float l = length(v);
    return v / l;
}

inline vec4 normalize(const vec4& v) {
    float l = length(v);
    return v / l;
}



inline float min(float a, float b) {
    return std::min(a, b);
}

inline vec2 min(const vec2& a, const vec2& b) {
    vec2 n(std::min(a.x, b.x), std::min(a.y, b.y));
    return n;
}

inline vec2 min(const vec2& a, float b) {
    vec2 n(std::min(a.x, b), std::min(a.y, b));
    return n;
}

inline vec3 min(const vec3& a, const vec3& b) {
    vec3 n(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    return n;
}

inline vec3 min(const vec3& a, float b) {
    vec3 n(std::min(a.x, b), std::min(a.y, b), std::min(a.z, b));
    return n;
}

inline vec4 min(const vec4& a, const vec4& b) {
    vec4 n(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w));
    return n;
}

inline vec4 min(const vec4& a, float b) {
    vec4 n(std::min(a.x, b), std::min(a.y, b), std::min(a.z, b), std::min(a.w, b));
    return n;
}


inline float max(float a, float b) {
    return std::max(a, b);
}

inline vec2 max(const vec2& a, const vec2& b) {
    vec2 n(std::max(a.x, b.x), std::max(a.y, b.y));
    return n;
}

inline vec2 max(const vec2& a, float b) {
    vec2 n(std::max(a.x, b), std::max(a.y, b));
    return n;
}

inline vec3 max(const vec3& a, const vec3& b) {
    vec3 n(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    return n;
}

inline vec3 max(const vec3& a, float b) {
    vec3 n(std::max(a.x, b), std::max(a.y, b), std::max(a.z, b));
    return n;
}

inline vec4 max(const vec4& a, const vec4& b) {
    vec4 n(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w));
    return n;
}

inline vec4 max(const vec4& a, float b) {
    vec4 n(std::max(a.x, b), std::max(a.y, b), std::max(a.z, b), std::max(a.w, b));
    return n;
}


inline float clamp(float val, float min_val, float max_val) {
    return max(min(val, min_val), max_val);
}

inline vec2 clamp(const vec2& v, float min_val, float max_val) {
    return max(min(v, min_val), max_val);
}

inline vec3 clamp(const vec3& v, float min_val, float max_val) {
    return max(min(v, min_val), max_val);
}

inline vec4 clamp(const vec4& v, float min_val, float max_val) {
    return max(min(v, min_val), max_val);
}


inline vec2 abs(const vec2& v) {
    vec2 n(std::abs(v.x), std::abs(v.y));
    return n;
}

inline vec3 abs(const vec3& v) {
    vec3 n(std::abs(v.x), std::abs(v.y), std::abs(v.z));
    return n;
}

inline vec4 abs(const vec4& v) {
    vec4 n(std::abs(v.x), std::abs(v.y), std::abs(v.z), std::abs(v.w));
    return n;
}


// inline float floor(float x) {
//     return std::floor(x);
// }

inline vec2 floor(const vec2& v) {
    vec2 n(std::floor(v.x), std::floor(v.y));
    return n;
}

inline vec3 floor(const vec3& v) {
    vec3 n(std::floor(v.x), std::floor(v.y), std::floor(v.z));
    return n;
}

inline vec4 floor(const vec4& v) {
    vec4 n(std::floor(v.x), std::floor(v.y), std::floor(v.z), std::floor(v.w));
    return n;
}


// inline float ceil(float x) {
//     return std::ceil(x);
// }

inline vec2 ceil(const vec2& v) {
    vec2 n(std::ceil(v.x), std::ceil(v.y));
    return n;
}

inline vec3 ceil(const vec3& v) {
    vec3 n(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z));
    return n;
}

inline vec4 ceil(const vec4& v) {
    vec4 n(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z), std::ceil(v.w));
    return n;
}


inline float fract(float x) {
    return x - floor(x);
}

inline vec2 fract(const vec2& v) {
    return v - floor(v);
}

inline vec3 fract(const vec3& v) {
    return v - floor(v);
}

inline vec4 fract(const vec4& v) {
    return v - floor(v);
}


inline int sign(float x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

inline vec2 sign(const vec2& v) {
    vec2 n(sign(v.x), sign(v.y));
    return n;
}

inline vec3 sign(const vec3& v) {
    vec3 n(sign(v.x), sign(v.y), sign(v.z));
    return n;
}

inline vec4 sign(const vec4& v) {
    vec4 n(sign(v.x), sign(v.y), sign(v.z), sign(v.w));
    return n;
}


inline float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}

inline vec2 smoothstep(float edge0, float edge1, const vec2& v) {
    vec2 n(smoothstep(edge0, edge1,v.x), smoothstep(edge0, edge1,v.y));
    return n;
}

inline vec3 smoothstep(float edge0, float edge1, const vec3& v) {
    vec3 n(smoothstep(edge0, edge1,v.x), smoothstep(edge0, edge1,v.y), smoothstep(edge0, edge1,v.z));
    return n;
}

inline vec4 smoothstep(float edge0, float edge1, const vec4& v) {
    vec4 n(smoothstep(edge0, edge1,v.x), smoothstep(edge0, edge1,v.y), smoothstep(edge0, edge1,v.z), smoothstep(edge0, edge1,v.w));
    return n;
}


inline float radians(float x) {
    return x * pi / 180;
}

inline vec2 radians(const vec2& v) {
    vec2 n(radians(v.x), radians(v.y));
    return n;
}

inline vec3 radians(const vec3& v) {
    vec3 n(radians(v.x), radians(v.y), radians(v.z));
    return n;
}

inline vec4 radians(const vec4& v) {
    vec4 n(radians(v.x), radians(v.y), radians(v.z), radians(v.w));
    return n;
}


inline float degrees(float x) {
    return (x * 180) / pi;
}

inline vec2 degrees(const vec2& v) {
    vec2 n(degrees(v.x), degrees(v.y));
    return n;
}

inline vec3 degrees(const vec3& v) {
    vec3 n(degrees(v.x), degrees(v.y), degrees(v.z));
    return n;
}

inline vec4 degrees(const vec4& v) {
    vec4 n(degrees(v.x), degrees(v.y), degrees(v.z), degrees(v.w));
    return n;
}


// inline float sin(float x) {
//     return std::sin(x);
// }

// inline float cos(float x) {
//     return std::cos(x);
// }

// inline float tan(float x) {
//     return std::tan(x);
// }


inline float mix(float x, float y, float a) {
    return x * (1.0 - a) + y * a;
}


// inline float sqrt(float x) {
//     return std::sqrt(x);
// }

inline vec2 sqrt(const vec2& v) {
    return vec2(sqrt(v.x), sqrt(v.y));
}

inline vec3 sqrt(const vec3& v) {
    return vec3(sqrt(v.x), sqrt(v.y), sqrt(v.z));
}

inline vec4 sqrt(const vec4& v) {
    return vec4(sqrt(v.x), sqrt(v.y), sqrt(v.z), sqrt(v.w));
}

#endif // ESSENTIALS_HPP