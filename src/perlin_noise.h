#pragma once

#include <random>

class PerlinNoise2D {
public:

    /// Initializes with a randomized hash table using `gen` to produce
    /// different results per instance.
    ///
    PerlinNoise2D(std::mt19937& gen);

    /// @return value in the range (-1, 1).
    ///
    float sample(float x, float y);

    static std::vector<int> get_random_gradiant_hashes(std::mt19937& gen);

private:

    static const int GRID_CELL_COUNT = 128; //64;
    static const int GRID_CELL_MASK = GRID_CELL_COUNT - 1;
    static const int GRID_VECTOR_WIDTH_COUNT = GRID_CELL_COUNT + 1;
    static const int GRID_SIZE = GRID_VECTOR_WIDTH_COUNT * GRID_VECTOR_WIDTH_COUNT;

    // Gradiant hashes table.
    int32_t m_hashes[GRID_SIZE];

    // Computes the dot product with one of 8 possible gradient vectors:
    // (1,2)  (-1,2)  (1,-2)  (-1,-2)  (2,1)  (2,-1)  (-2,1)  (-2,-1)
    static float grad(int32_t hash, float x, float y);

};

class PerlinNoise3D {
public:

    /// Initializes with a randomized permutation table using `gen` to produce
    /// different results per instance.
    ///
    PerlinNoise3D(std::mt19937& gen);

    /// @return value in the range (-1, 1).
    ///
    float sample(float x, float y, float z);

private:

    // uint8_t makes for fewer cache misses over int32_t.
    uint8_t m_perm[512];

    // Uses the lower 16 bits of `hash` to randomly choose from 12 gradient
    // vectors with 4 duplicates.
    static float grad(int32_t hash, float x, float y, float z);

    int32_t perm(int32_t i);

};
