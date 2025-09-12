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
    half result;
    result.bits = h.bits & 0x7FFF;
    return result;
}

inline half sin(const half& h) {
    half result;
    uint32_t x_fp = (h.bits & 0x7FFF) << 5;

    uint32_t x2 = (x_fp * x_fp) >> 10;
    uint32_t x3 = (x2 * x_fp) >> 10;
    uint32_t x5 = (x3 * x2) >> 10;

    uint32_t approx = x_fp - (x3 / 6) + (x5 / 120);
    result.bits = approx >> 5;
    return result;
}

inline half cos(const half& h) {
    half result;
    uint32_t x_fp = (h.bits & 0x7FFF) << 5;

    uint32_t x2 = (x_fp * x_fp) >> 10;
    uint32_t x4 = (x2 * x2) >> 10;

    uint32_t approx = (1 << 15) - (x2 >> 1) + (x4 / 24);
    result.bits = approx >> 5;
    return result;
}

inline half tan(const half& h) {
    half result;
    uint32_t x_fp = (h.bits & 0x7FFF) << 5;

    uint32_t x2 = (x_fp * x_fp) >> 10;
    uint32_t x3 = (x2 * x_fp) >> 10;
    uint32_t x5 = (x3 * x2) >> 10;

    uint32_t approx = x_fp + (x3 / 3) + (2 * x5 / 15);
    result.bits = approx >> 5;
    return result;
}

inline half exp(const half& h) {
    half result;
    uint16_t x = h.bits & 0x7FFF;

    // crude scale: treat bits as small fixed-point
    uint32_t x_fp = x << 5; // scale up for precision

    uint32_t x2 = (x_fp * x_fp) >> 10;
    uint32_t x3 = (x2 * x_fp) >> 10;

    uint32_t approx = (1 << 15) + x_fp + (x2 >> 1) + (x3 / 6);
    result.bits = approx >> 5;
    return result;
}

inline half log(const half& h) {
    half result;
    if ((h.bits & 0x7FFF) == 0) {
        result.bits = 0; // log(0) = undefined → return 0
        return result;
    }

    uint32_t x_fp = (h.bits & 0x7FFF) << 5;
    uint32_t x_minus_1 = x_fp - (1 << 15);

    uint32_t x2 = (x_minus_1 * x_minus_1) >> 10;
    uint32_t x3 = (x2 * x_minus_1) >> 10;

    uint32_t approx = x_minus_1 - (x2 >> 1) + (x3 / 3);
    result.bits = approx >> 5;
    return result;
}

inline half floor(const half& h) {
    half result = h;
    uint16_t exp = (h.bits >> 10) & 0x1F;

    if (exp < 10) {
        uint16_t mask = (1 << (10 - exp)) - 1;
        result.bits &= ~mask;
    }

    return result;
}

inline half ceil(const half& h) {
    half result = floor(h);
    if ((result.bits != h.bits) && ((h.bits & 0x8000) == 0)) {
        // Add 1 if positive and not already an integer
        result.bits += 1;
    }
    return result;
}

inline half round(const half& h) {
    half result = h;
    uint16_t exp = (h.bits >> 10) & 0x1F;

    if (exp < 10) {
        uint16_t mask = (1 << (10 - exp)) - 1;
        uint16_t frac = h.bits & mask;
        result.bits &= ~mask;

        if (frac > (mask >> 1)) {
            result.bits += (1 << (10 - exp));
        }
    }

    return result;
}

inline half fmod(const half& a, const half& b) {
    half result;
    if ((b.bits & 0x7FFF) == 0) {
        result.bits = 0; // divide by zero → return 0
        return result;
    }

    uint16_t rem = a.bits % b.bits;
    result.bits = rem;
    return result;
}

#endif