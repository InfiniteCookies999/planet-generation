#pragma once

#include <random>

class PerlinNoise2D {
public:

    PerlinNoise2D(std::mt19937& _gen);

    double sample(double x, double y);

    static std::vector<int> get_random_gradiant_hashes(std::mt19937& gen);

private:

    static const int GRID_CELL_COUNT = 128; //64;
    static const int GRID_CELL_MASK = GRID_CELL_COUNT - 1;
    static const int GRID_VECTOR_WIDTH_COUNT = GRID_CELL_COUNT + 1;
    static const int GRID_SIZE = GRID_VECTOR_WIDTH_COUNT * GRID_VECTOR_WIDTH_COUNT;

    // Gradiant hashes table.
    int m_hashes[GRID_SIZE];

    static double grad(int hash, double x, double y);

};

class PerlinNoise3D {
public:

    PerlinNoise3D(std::mt19937& gen);

    double sample(double x, double y, double z);

private:

    int m_permutations[512];

    // Uses the lower 16 bits of `hash` to randomly choose from 12 gradient
    // vectors with 4 duplicates.
    static double grad(int hash, double x, double y, double z);

};
