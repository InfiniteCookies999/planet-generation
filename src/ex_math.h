#pragma once

#include <math.h>

inline float degrees(float radians) {
    return radians * (180.f / M_PI);
}

inline float radians(float degrees) {
    return degrees * (M_PI / 180.0f);
}
