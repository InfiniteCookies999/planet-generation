#include "star.h"
#include <random>

Star create_randomized_star(std::mt19937& gen) {
    constexpr int STAR_COLOR_COUNT = static_cast<int>(StarColor::STAR_COLOR_COUNT);
    std::uniform_int_distribution<> color_dist(0, STAR_COLOR_COUNT - 1);

    StarColor color = static_cast<StarColor>(color_dist(gen));

    return Star{
        .position = { 0, 0, 0 },
        .radius = 4.0f,
        .color = color
    };
}