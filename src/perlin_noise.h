#pragma once

#include <random>
class PerlinNoise {
public:

    PerlinNoise(std::mt19937& gen);

    double sample(double x, double y);

    static std::vector<int> get_random_gradiant_hashes(std::mt19937& gen);

private:

    static const int GRID_CELL_COUNT = 128; //64;
    static const int GRID_CELL_MASK = GRID_CELL_COUNT - 1;
    static const int GRID_VECTOR_WIDTH_COUNT = GRID_CELL_COUNT + 1;
    static const int GRID_SIZE = GRID_VECTOR_WIDTH_COUNT * GRID_VECTOR_WIDTH_COUNT;

    // Gradiant hashes table.
    int hashes[GRID_SIZE];

};
