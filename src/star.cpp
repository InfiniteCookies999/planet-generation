#include "star.h"
#include <random>
#include <stdexcept>

/**
 * Determines the size scaling factor for a star given its color range.
 */
float get_random_scale_factor(StarColor color, std::mt19937& gen) {
    switch (color) {
    case StarColor::STAR_WHITE: {
        std::uniform_real_distribution<float> dist(1.15f, 6.6f);
        return dist(gen);
    }
    case StarColor::STAR_YELLOW: {
        std::uniform_real_distribution<float> dist(0.96f, 1.15f);
        return dist(gen);
    }
    case StarColor::STAR_ORANGE: {
        std::uniform_real_distribution<float> dist(0.7f, 0.96f);
        return dist(gen);
    }
    case StarColor::STAR_RED: {
        std::uniform_real_distribution<float> dist(0.4f, 0.7f);
        return dist(gen);
    }
    default:
        throw std::invalid_argument("Invalid star color");
    }
}

Star create_randomized_star(std::mt19937& gen) {
    constexpr int STAR_COLOR_COUNT = static_cast<int>(StarColor::STAR_COLOR_COUNT);
    std::uniform_int_distribution<> color_dist(0, STAR_COLOR_COUNT - 1);

    StarColor color = static_cast<StarColor>(color_dist(gen));

    float scale_factor = get_random_scale_factor(color, gen);

    return Star{
        .position = { 0, 0, 0 }, //{ 20, 10, 20 },
        .radius = 4.5f * scale_factor,
        .color = color
    };
}