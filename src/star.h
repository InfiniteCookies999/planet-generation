#pragma once

#include <random>
#include <raylib.h>

enum class StarColor {
    STAR_WHITE,
    STAR_ORANGE,
    STAR_YELLOW,
    STAR_RED,
    STAR_COLOR_COUNT
};

struct Star {
    Vector3 position;
    float radius;
    StarColor color;
};

Star create_randomized_star(std::mt19937& gen);
