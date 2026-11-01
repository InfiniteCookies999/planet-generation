#include "perlin_noise.h"

#include <random>
#include <raymath.h>

static inline float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static inline int fast_floor(float v) {
    int i = static_cast<int>(v);
    return (v < i) ? (i - 1) : (i);
}


PerlinNoise2D::PerlinNoise2D(std::mt19937& gen) {
    std::uniform_int_distribution<int32_t> dist(0, 7);
    // Randomize the hashes table.
    for (int i = 0; i < GRID_SIZE; i++) {
        m_hashes[i] = dist(gen);
    }
}

std::vector<int> PerlinNoise2D::get_random_gradiant_hashes(std::mt19937& gen) {
    std::vector<int> hashes(GRID_SIZE);
    std::uniform_int_distribution<> dist(0, 7);
    // Randomize the hashes table.
    for (int i = 0; i < GRID_SIZE; i++) {
        hashes[i] = dist(gen);
    }
    return hashes;
}

float PerlinNoise2D::sample(float x, float y) {

    // Find the cell that the point x,y lies in.
    int cell_x = static_cast<int>(fast_floor(x)) & GRID_CELL_MASK;
    int cell_y = static_cast<int>(fast_floor(y)) & GRID_CELL_MASK;

    // Sample the grid of gradient hashes.
    int32_t gbl = m_hashes[cell_y*GRID_VECTOR_WIDTH_COUNT + cell_x];
    int32_t gbr = m_hashes[cell_y*GRID_VECTOR_WIDTH_COUNT + (cell_x+1)];
    int32_t gtl = m_hashes[(cell_y+1)*GRID_VECTOR_WIDTH_COUNT + cell_x];
    int32_t gtr = m_hashes[(cell_y+1)*GRID_VECTOR_WIDTH_COUNT + (cell_x+1)];

    // Distance of x/y in cell.
    float dx = x - fast_floor(x);
    float dy = y - fast_floor(y);

    // Computing dot product of distance vectors against gradient vectors.
    float bl = grad(gbl, dx    , dy    );
    float br = grad(gbr, dx - 1, dy    );
    float tl = grad(gtl, dx    , dy - 1);
    float tr = grad(gtr, dx - 1, dy - 1);

    // Interpolate the contribution of each vector along the X and Y axis.

    // Use of fade here makes it so when the sample points are close to the edges
    // of the cells it smoothly fades between them instead of having a harsh cutoff.
    float tx = fade(dx);
    float ty = fade(dy);
    return Lerp(Lerp(bl, br, tx), Lerp(tl, tr, tx), ty);
}

float PerlinNoise2D::grad(int32_t hash, float x, float y) {
    int32_t h = hash & 0x7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}


PerlinNoise3D::PerlinNoise3D(std::mt19937& gen) {

    // Randomize the permutation table.
    // Uses Fisher-Yates Shuffle Modern Algorithm.
    for (int i = 0; i < 256; i++) {
        m_perm[i] = static_cast<uint8_t>(i);
    }

    for (int i = 256 - 1; i > 0; i--) {
        std::uniform_int_distribution<int> dist(0, i);
        int k = dist(gen);
        uint8_t t = m_perm[i];
        m_perm[i] = m_perm[k];
        m_perm[k] = t;
    }

    for (int i = 0; i < 256; i++) {
        m_perm[i+256] = m_perm[i];
    }
}

float PerlinNoise3D::sample(float x, float y, float z) {

    // Find cell that the point x,y,z lies in.
    int32_t cell_x = static_cast<int32_t>(fast_floor(x)) & 255;
    int32_t cell_y = static_cast<int32_t>(fast_floor(y)) & 255;
    int32_t cell_z = static_cast<int32_t>(fast_floor(z)) & 255;

    float dx = x - static_cast<int32_t>(fast_floor(x));
    float dy = y - static_cast<int32_t>(fast_floor(y));
    float dz = z - static_cast<int32_t>(fast_floor(z));

    // Computing permutation hashes for all corners of the cube.
    int32_t px0  = perm(cell_x);
    int32_t px1  = perm(cell_x+1);
    int32_t py00 = perm(px0 + cell_y);
    int32_t py01 = perm(px0 + cell_y+1);
    int32_t py10 = perm(px1 + cell_y);
    int32_t py11 = perm(px1 + cell_y+1);

    int32_t h000 = perm(py00 + cell_z  );
    int32_t h100 = perm(py10 + cell_z  );
    int32_t h010 = perm(py01 + cell_z  );
    int32_t h001 = perm(py00 + cell_z+1);
    int32_t h110 = perm(py11 + cell_z  );
    int32_t h101 = perm(py10 + cell_z+1);
    int32_t h011 = perm(py01 + cell_z+1);
    int32_t h111 = perm(py11 + cell_z+1);


    float g000 = grad(h000, dx    , dy    , dz    );
    float g100 = grad(h100, dx - 1, dy    , dz    );
    float g010 = grad(h010, dx    , dy - 1, dz    );
    float g001 = grad(h001, dx    , dy    , dz - 1);
    float g110 = grad(h110, dx - 1, dy - 1, dz    );
    float g101 = grad(h101, dx - 1, dy    , dz - 1);
    float g011 = grad(h011, dx    , dy - 1, dz - 1);
    float g111 = grad(h111, dx - 1, dy - 1, dz - 1);

    // Trilinearly interpolating on the X,Y,Z axes.
    float tx = fade(dx);
    float ty = fade(dy);
    float tz = fade(dz);

    float x1 = Lerp(g000, g100, tx);
    float x2 = Lerp(g010, g110, tx);
    float x3 = Lerp(g001, g101, tx);
    float x4 = Lerp(g011, g111, tx);

    float y1 = Lerp(x1, x2, ty);
    float y2 = Lerp(x3, x4, ty);

    return Lerp(y1, y2, tz);
}

float PerlinNoise3D::grad(int32_t hash, float x, float y, float z) {
    hash &= 15;
    float v0 = hash < 8 ? x : y;
    float v1 = hash < 4 ? y : (hash == 12 || hash == 14) ? x : z;
    v0 = (hash & 1) == 0 ? v0 : -v0;
    v1 = (hash & 2) == 0 ? v1 : -v1;
    return v0 + v1;
}

int32_t PerlinNoise3D::perm(int32_t i) {
    return static_cast<int32_t>(m_perm[i]);
}
