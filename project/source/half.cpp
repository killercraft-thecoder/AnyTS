#include "half.h"
#ifdef ADD_STD_HALF
// Conversion helpers
inline uint16_t half::float_to_half(float f)
{
    uint32_t x = *reinterpret_cast<uint32_t *>(&f);
    uint32_t sign = (x >> 16) & 0x8000;
    int32_t exponent = ((x >> 23) & 0xFF) - 127 + 15;
    uint32_t mantissa = x & 0x007FFFFF;

    if (exponent <= 0)
    {
        // Subnormal or zero
        if (exponent < -10)
            return sign;        // too small becomes zero
        mantissa |= 0x00800000; // add implicit leading 1
        mantissa >>= (1 - exponent);
        return sign | (mantissa >> 13);
    }
    else if (exponent >= 31)
    {
        // Overflow to infinity or NaN
        if (mantissa == 0)
            return sign | 0x7C00;                // infinity
        return sign | 0x7C00 | (mantissa >> 13); // NaN
    }

    // Normalized number
    return sign | (exponent << 10) | (mantissa >> 13);
}

inline float half::half_to_float(uint16_t h)
{
    uint32_t sign = (h & 0x8000) << 16;
    uint32_t exponent = (h >> 10) & 0x1F;
    uint32_t mantissa = h & 0x3FF;

    if (exponent == 0)
    {
        if (mantissa == 0)
        {
            // Zero
            uint32_t result = sign;
            return *reinterpret_cast<float *>(&result);
        }
        // Subnormal
        exponent = 1;
        while ((mantissa & 0x400) == 0)
        {
            mantissa <<= 1;
            exponent--;
        }
        mantissa &= 0x3FF;
        exponent += 127 - 15;
        uint32_t result = sign | (exponent << 23) | (mantissa << 13);
        return *reinterpret_cast<float *>(&result);
    }
    else if (exponent == 31)
    {
        // Inf or NaN
        uint32_t result = sign | 0x7F800000 | (mantissa << 13);
        return *reinterpret_cast<float *>(&result);
    }

    // Normalized
    exponent = exponent + 127 - 15;
    uint32_t result = sign | (exponent << 23) | (mantissa << 13);
    return *reinterpret_cast<float *>(&result);
}

// Constructors
half::half() : bits(0) {}
half::half(float f) : bits(float_to_half(f)) {}
half::half(double f) : bits(float_to_half(static_cast<float>(f))) {}
half::operator float() const { return half_to_float(bits); }

// Arithmetic
half half::operator+(const half &other) const
{
    float a = half_to_float(bits);
    float b = half_to_float(other.bits);
    return half(a + b);
}

half half::operator-(const half &other) const
{
    float a = half_to_float(bits);
    float b = half_to_float(other.bits);
    return half(a - b);
}

half half::operator*(const half &other) const
{
    float a = half_to_float(bits);
    float b = half_to_float(other.bits);
    return half(a * b);
}

half half::operator/(const half &other) const
{
    float a = half_to_float(bits);
    float b = half_to_float(other.bits);
    return half(a / b);
}

half half::operator%(const half &b) const
{
    return half(std::fmod(half_to_float(bits), half_to_float(b.bits)));
}

// Comparison
bool half::operator==(const half &other) const { return float(*this) == float(other); }
bool half::operator!=(const half &other) const { return float(*this) != float(other); }
bool half::operator<(const half &other) const { return float(*this) < float(other); }
bool half::operator>(const half &other) const { return float(*this) > float(other); }
bool half::operator<=(const half &other) const { return float(*this) <= float(other); }
bool half::operator>=(const half &other) const { return float(*this) >= float(other); }

inline half sqrt(const half &h)
{
    return half(std::sqrt(half::half_to_float(h.bits)));
}

inline half abs(const half &h)
{
    return half(std::fabs(half::half_to_float(h.bits)));
}

inline half sin(const half &h)
{
    return half(std::sin(half::half_to_float(h.bits)));
}

inline half cos(const half &h)
{
    return half(std::cos(half::half_to_float(h.bits)));
}

inline half tan(const half &h)
{
    return half(std::tan(half::half_to_float(h.bits)));
}

inline half exp(const half &h)
{
    return half(std::exp(half::half_to_float(h.bits)));
}

inline half log(const half &h)
{
    return half(std::log(half::half_to_float(h.bits)));
}

inline half floor(const half &h)
{
    return half(std::floor(half::half_to_float(h.bits)));
}

inline half ceil(const half &h)
{
    return half(std::ceil(half::half_to_float(h.bits)));
}

inline half round(const half &h)
{
    return half(std::round(half::half_to_float(h.bits)));
}

inline half fmod(const half &a, const half &b)
{
    return half(std::fmod(half::half_to_float(a.bits), half::half_to_float(b.bits)));
}
#endif