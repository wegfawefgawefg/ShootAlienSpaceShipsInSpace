#pragma once

#include <cmath>

struct Vec2 {
    float x{0.0f};
    float y{0.0f};
};

inline Vec2 operator+(Vec2 a, Vec2 b) {
    return {a.x + b.x, a.y + b.y};
}

inline Vec2 operator-(Vec2 a, Vec2 b) {
    return {a.x - b.x, a.y - b.y};
}

inline Vec2 operator*(Vec2 a, float s) {
    return {a.x * s, a.y * s};
}

inline Vec2 operator*(float s, Vec2 a) {
    return a * s;
}

inline Vec2 lerp(Vec2 a, Vec2 b, float t) {
    return a + (b - a) * t;
}

inline Vec2& operator+=(Vec2& a, Vec2 b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

inline float vec2_length_sq(Vec2 v) {
    return v.x * v.x + v.y * v.y;
}

inline float vec2_length(Vec2 v) {
    return std::sqrt(vec2_length_sq(v));
}

inline float clampf(float value, float min_value, float max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}
