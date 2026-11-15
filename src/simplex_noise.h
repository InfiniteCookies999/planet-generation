#pragma once

#include <random>

class SimplexNoise2D {
public:

    /// Initializes with a randomized hash table using `gen` to produce
    /// different results per instance.
    ///
    SimplexNoise2D(std::mt19937& gen);

    /// @return value in the range (-1, 1).
    ///
    float sample(float x, float y);

private:
    static const int GRID_CELL_COUNT = 128;
    static const int GRID_CELL_MASK = GRID_CELL_COUNT - 1;
    static const int GRID_VECTOR_WIDTH_COUNT = GRID_CELL_COUNT + 1;
    static const int GRID_SIZE = GRID_VECTOR_WIDTH_COUNT * GRID_VECTOR_WIDTH_COUNT;

    // Gradiant hashes table.
    int32_t m_hashes[GRID_SIZE];

    static int wrap(int x);

    // Computes the dot product with one of 8 possible gradient vectors:
    // (1,2)  (-1,2)  (1,-2)  (-1,-2)  (2,1)  (2,-1)  (-2,1)  (-2,-1)
    static float grad(int32_t hash, float x, float y);

};

class SimplexNoise3D {
public:

    SimplexNoise3D(std::mt19937& gen);

    float sample(float x, float y, float z);

private:

    // uint8_t makes for fewer cache misses over int32_t.
    uint8_t m_perm[512];

    static int32_t wrap(int32_t x);

    int32_t perm(int32_t i);

    static float grad(int32_t hash, float x, float y, float z);

};
