//
// Created by etders on 09.04.2026.
//

#ifndef ARGUSENGINE_TILEFIXEDPOINT_HPP
#define ARGUSENGINE_TILEFIXEDPOINT_HPP
#include <cstdint>
#include <cstddef>
#include <cmath>

#include "Utils.hpp"

namespace ArgusEngine::Math {

    template<int FractionalBits = 16>
struct Fixed {
        int32_t raw;
        static constexpr int32_t scale = 1 << FractionalBits;

        Fixed() : raw(0) {}

        Fixed(int32_t val) : raw(val << FractionalBits) {}

        static Fixed FromRaw(int32_t rawVal) {
            Fixed f; f.raw = rawVal; return f;
        }

        Fixed& operator=(int32_t val) {
            raw = val << FractionalBits;
            return *this;
        }

        FORCE_INLINE Fixed operator>>(uint32_t offset) const {
            return FromRaw(raw >> offset);
        }
        FORCE_INLINE Fixed operator<<(uint32_t offset) const {
            return FromRaw(raw << offset);
        }

        float to_float() const { return static_cast<float>(raw) / scale; }
        int32_t to_int() const { return raw >> FractionalBits; }

        Fixed operator+(const Fixed& other) const {
            Fixed res; res.raw = raw + other.raw; return res;
        }

        Fixed operator-(const Fixed& other) const {
            Fixed res; res.raw = raw - other.raw; return res;
        }

        Fixed operator-() const {
            Fixed res;
            res.raw = -raw;
            return res;
        }

        Fixed operator*(const Fixed& other) const {
            Fixed res;
            res.raw = static_cast<int32_t>((static_cast<int64_t>(raw) * other.raw) >> FractionalBits);
            return res;
        }

        Fixed operator*(float other) const {
            return Fixed(to_int(to_float() * other));
        }

        Fixed operator/(const Fixed& other) const {
            Fixed res;
            if (other.raw == 0) [[unlikely]] {
                res.raw = 0;
                return res;
            }
            res.raw = static_cast<int32_t>((static_cast<int64_t>(raw) << FractionalBits) / other.raw);
            return res;
        }

        bool operator<(const Fixed& other) const { return raw < other.raw; }
        bool operator>(const Fixed& other) const { return raw > other.raw; }
        bool operator==(const Fixed& other) const { return raw == other.raw; }

        operator float() const {
            return static_cast<float>(raw) / (1 << FractionalBits);
        }

        operator int32_t() const {
            return raw >> FractionalBits;
        }
    };

    using Fixed32 = Fixed<24>;

    // TODO: переделать по нормальному, ибо сова на глобус, Кармак недоволен, ОНО НЕ БУДЕТ РАБОТАТЬ!!!!!

    // НЕ ИСПОЛЬЗОВАТЬ, СЛОМАЕТ ВСЕ!!!!
    inline Fixed32 fixed_inv_sqrt(Fixed32 a) {
        if (a.raw <= 0) return Fixed32(0.0f);

        Fixed32 x;
        x.raw = 0x5f3759df - (a.raw >> 1);

        static const Fixed32 threeHalves(1.5f);
        static const Fixed32 half(0.5f);

        Fixed32 half_a = a * half;
        x = x * (threeHalves - (half_a * x * x));

        return x;
    }

    inline Fixed32 fixed_sqrt(Fixed32 a) {
        if (a.raw <= 0) return Fixed32(0.0f);
        return a * fixed_inv_sqrt(a);
    }

    template<int FractionalBits = 16>
    struct Vector3Fixed {
        Fixed<FractionalBits> x, y, z;

        Vector3Fixed() : x(0), y(0), z(0) {}

        Vector3Fixed(Fixed<FractionalBits> x = 0,
            Fixed<FractionalBits> y = 0,
            Fixed<FractionalBits> z = 0) : x(x), y(y), z(z) {}

        Vector3Fixed(float x,
            float y,
            float z) : x(x), y(y), z(z) {}

        Vector3Fixed operator+(const Vector3Fixed& other) const {return Vector3Fixed(x + other.x, y + other.y, z + other.z); }
        Vector3Fixed operator-(const Vector3Fixed& other) const {return Vector3Fixed(x - other.x, y - other.y, z - other.z); }
        Vector3Fixed operator*(const Vector3Fixed& other) const {return Vector3Fixed(x * other.x, y * other.y, z * other.z); }
        Vector3Fixed operator/(const Vector3Fixed& other) const {return Vector3Fixed(x / other.x, y / other.y, z / other.z); }

        Vector3Fixed operator>>(uint32_t offset) const {
            return Vector3Fixed(x >> offset, y >> offset, z >> offset);
        }

        Vector3Fixed operator<<(uint32_t offset) const {
            return Vector3Fixed(x << offset, y << offset, z << offset);
        }

        Fixed<FractionalBits> dot(const Vector3Fixed& other) const {
            return (x * other.x) + (y * other.y) + (z * other.z);
        }

        Fixed<FractionalBits> lengthSq() const {
            return dot(*this);
        }

        static Vector3Fixed lerp(const Vector3Fixed& a, const Vector3Fixed& b, Fixed<FractionalBits> t) {
            return a + (b - a) * t;
        }

        Fixed<FractionalBits> fastLength() const {
            int32_t ax = std::abs(x.raw);
            int32_t ay = std::abs(y.raw);
            int32_t az = std::abs(z.raw);

            int32_t max_v = std::max(ax, std::max(ay, az));
            int32_t sum_others = (ax + ay + az) - max_v;

            Fixed<FractionalBits> res;
            // max + (sum_others / 4)
            res.raw = max_v + (sum_others >> 2);
            return res;
        }
    };

    using vec3 = Vector3Fixed<24>;

    struct alignas(16) QuatFixed {
        Fixed32 x, y, z, w;

        QuatFixed() : x(0), y(0), z(0), w(Fixed32(1.0f)) {}

        QuatFixed(Fixed32 _x, Fixed32 _y, Fixed32 _z, Fixed32 _w)
            : x(_x), y(_y), z(_z), w(_w) {}

        QuatFixed operator*(const QuatFixed& q) const {
            return QuatFixed(
                w * q.x + x * q.w + y * q.z - z * q.y,
                w * q.y + y * q.w + z * q.x - x * q.z,
                w * q.z + z * q.w + x * q.y - y * q.x,
                w * q.w - x * q.x - y * q.y - z * q.z
            );
        }

        Vector3Fixed<24> rotate(const Vector3Fixed<24> & v) const {
            QuatFixed vQuat(v.x, v.y, v.z, Fixed32(0.0f));
            QuatFixed conj(-x, -y, -z, w);
            QuatFixed res = (*this * vQuat) * conj;
            return Vector3Fixed<24>(res.x, res.y, res.z);
        }

        // TODO: ПЕРЕДЕЛАТЬ ОБРАТНЫЙ КВАДРАТНЫЙ КОРЕНЬ!!!
        void normalize() {
            Fixed32 magSq = x*x + y*y + z*z + w*w;
            // TODO: БЛЯТЬ ПЕРЕДЕЛАТЬ ЭТО ОБЯЗАТЕЛЬНО!!!
            Fixed32 invMag = fixed_inv_sqrt(magSq);
            x = x * invMag;
            y = y * invMag;
            z = z * invMag;
            w = w * invMag;
        }
    };
}

#endif //ARGUSENGINE_TILEFIXEDPOINT_HPP
