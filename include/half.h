#pragma once
#include <cstdint>
#include <cmath>

class half
{
public:
    uint16_t bits;
    static inline uint16_t float_to_half(float f);
    static inline float half_to_float(uint16_t h);
    half();        // default constructor
    half(float f); // construct from float
    half::half(double f);
    operator float() const; // convert to float

    // Arithmetic operators
    half operator+(const half &other) const;
    half operator-(const half &other) const;
    half operator*(const half &other) const;
    half operator/(const half &other) const;
    half operator%(const half &b) const;

    // Comparison operators
    bool operator==(const half &other) const;
    bool operator!=(const half &other) const;
    bool operator<(const half &other) const;
    bool operator>(const half &other) const;
    bool operator<=(const half &other) const;
    bool operator>=(const half &other) const;
};

// Math overloads in global namespace
inline half sqrt(const half &h);
inline half abs(const half &h);
inline half sin(const half &h);
inline half cos(const half &h);
inline half tan(const half &h);
inline half exp(const half &h);
inline half log(const half &h);
inline half floor(const half &h);
inline half ceil(const half &h);
inline half round(const half &h);
inline half fmod(const half &a, const half &b);

// half vs int
inline bool operator==(const half &h, int i) { return float(h) == static_cast<float>(i); }
inline bool operator!=(const half &h, int i) { return float(h) != static_cast<float>(i); }
inline bool operator<(const half &h, int i) { return float(h) < static_cast<float>(i); }
inline bool operator>(const half &h, int i) { return float(h) > static_cast<float>(i); }
inline bool operator<=(const half &h, int i) { return float(h) <= static_cast<float>(i); }
inline bool operator>=(const half &h, int i) { return float(h) >= static_cast<float>(i); }

// int vs half (for symmetry)
inline bool operator==(int i, const half &h) { return static_cast<float>(i) == float(h); }
inline bool operator!=(int i, const half &h) { return static_cast<float>(i) != float(h); }
inline bool operator<(int i, const half &h) { return static_cast<float>(i) < float(h); }
inline bool operator>(int i, const half &h) { return static_cast<float>(i) > float(h); }
inline bool operator<=(int i, const half &h) { return static_cast<float>(i) <= float(h); }
inline bool operator>=(int i, const half &h) { return static_cast<float>(i) >= float(h); }

// half + int
inline half operator+(const half &h, int i)
{
    return half(float(h) + static_cast<float>(i));
}

// int + half
inline half operator+(int i, const half &h)
{
    return half(static_cast<float>(i) + float(h));
}

// half - int
inline half operator-(const half &h, int i)
{
    return half(float(h) - static_cast<float>(i));
}

// int - half
inline half operator-(int i, const half &h)
{
    return half(static_cast<float>(i) - float(h));
}

// half * float
inline half operator*(const half& h, float f) {
    return half(float(h) * f);
}
inline half operator*(float f, const half& h) {
    return half(f * float(h));
}

// half - float
inline half operator-(const half& h, float f) {
    return half(float(h) - f);
}
inline half operator-(float f, const half& h) {
    return half(f - float(h));
}

// half += half
inline half& operator+=(half& lhs, const half& rhs) {
    lhs = lhs + rhs;
    return lhs;
}

// half -= half
inline half& operator-=(half& lhs, const half& rhs) {
    lhs = lhs - rhs;
    return lhs;
}