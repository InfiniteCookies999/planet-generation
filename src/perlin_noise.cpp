#include "perlin_noise.h"

#include <cmath>
#include <random>
#include <raymath.h>

static double fade(double t);

PerlinNoise2D::PerlinNoise2D(std::mt19937& gen) {
    std::uniform_int_distribution<> dist(0, 3);
    // Randomize the hashes table.
    for (int i = 0; i < GRID_SIZE; i++) {
        m_hashes[i] = dist(gen);
    }
}

std::vector<int> PerlinNoise2D::get_random_gradiant_hashes(std::mt19937& _gen) {
    std::vector<int> hashes(GRID_SIZE);
    std::uniform_int_distribution<> dist(0, 3);
    // Randomize the hashes table.
    for (int i = 0; i < GRID_SIZE; i++) {
        hashes[i] = dist(_gen);
    }
    return hashes;
}

double PerlinNoise2D::sample(double x, double y) {

    // Find the cell that the point x,y lies in.
    int cell_x = static_cast<int>(floor(x)) & GRID_CELL_MASK;
    int cell_y = static_cast<int>(floor(y)) & GRID_CELL_MASK;

    // Sample the grid of gradient hashes.
    int gbl = m_hashes[cell_y*GRID_VECTOR_WIDTH_COUNT + cell_x];
    int gbr = m_hashes[cell_y*GRID_VECTOR_WIDTH_COUNT + (cell_x+1)];
    int gtl = m_hashes[(cell_y+1)*GRID_VECTOR_WIDTH_COUNT + cell_x];
    int gtr = m_hashes[(cell_y+1)*GRID_VECTOR_WIDTH_COUNT + (cell_x+1)];

    // Distance of x/y in cell.
    double dx = x - floor(x);
    double dy = y - floor(y);

    // Computing dot product of distance vectors against gradient vectors.
    double bl = grad(gbl, dx    , dy    );
    double br = grad(gbr, dx - 1, dy    );
    double tl = grad(gtl, dx    , dy - 1);
    double tr = grad(gtr, dx - 1, dy - 1);

    // Interpolate the contribution of each vector along the X and Y axis.

    // Use of fade here makes it so when the sample points are close to the edges
    // of the cells it smoothly fades between them instead of having a harsh cutoff.
    double tx = fade(dx);
    double ty = fade(dy);
    return Lerp(Lerp(bl, br, tx), Lerp(tl, tr, tx), ty);
}

double PerlinNoise2D::grad(int hash, double x, double y) {
    if      (hash == 0) return x;  // dot product with ( 1,  0)
    else if (hash == 1) return -x; // dot product with (-1,  0)
    else if (hash == 2) return y;  // dot product with ( 0,  1)
    else                return -y; // dot product with ( 0, -1)
}


PerlinNoise3D::PerlinNoise3D(std::mt19937& gen) {

    // Randomize the permutation table.
    // Uses Fisher-Yates Shuffle Modern Algorithm.
    for (int i = 0; i < 256; i++) {
        m_permutations[i] = i;
    }

    for (int i = 256 - 1; i > 0; i--) {
        std::uniform_int_distribution<> dist(0, i);
        int k = dist(gen);
        int t = m_permutations[i];
        m_permutations[i] = m_permutations[k];
        m_permutations[k] = t;
    }

    for (int i = 0; i < 256; i++) {
        m_permutations[i+256] = m_permutations[i];
    }
}

double PerlinNoise3D::sample(double x, double y, double z) {

    // Find cell that the point x,y,z lies in.
    int cell_x = static_cast<int>(floor(x)) & 255;
    int cell_y = static_cast<int>(floor(y)) & 255;
    int cell_z = static_cast<int>(floor(z)) & 255;

    double dx = x - static_cast<int>(floor(x));
    double dy = y - static_cast<int>(floor(y));
    double dz = z - static_cast<int>(floor(z));

    // Computing permutation hashes for all corners of the cube.
    int px0  = m_permutations[cell_x];
    int px1  = m_permutations[cell_x+1];
    int py00 = m_permutations[px0 + cell_y];
    int py01 = m_permutations[px0 + cell_y+1];
    int py10 = m_permutations[px1 + cell_y];
    int py11 = m_permutations[px1 + cell_y+1];

    int h000 = m_permutations[py00 + cell_z  ];
    int h100 = m_permutations[py10 + cell_z  ];
    int h010 = m_permutations[py01 + cell_z  ];
    int h001 = m_permutations[py00 + cell_z+1];
    int h110 = m_permutations[py11 + cell_z  ];
    int h101 = m_permutations[py10 + cell_z+1];
    int h011 = m_permutations[py01 + cell_z+1];
    int h111 = m_permutations[py11 + cell_z+1];

    double g000 = grad(h000, dx    , dy    , dz    );
    double g100 = grad(h100, dx - 1, dy    , dz    );
    double g010 = grad(h010, dx    , dy - 1, dz    );
    double g001 = grad(h001, dx    , dy    , dz - 1);
    double g110 = grad(h110, dx - 1, dy - 1, dz    );
    double g101 = grad(h101, dx - 1, dy    , dz - 1);
    double g011 = grad(h011, dx    , dy - 1, dz - 1);
    double g111 = grad(h111, dx - 1, dy - 1, dz - 1);

    // Trilinearly interpolating on the X,Y,Z axes.
    double tx = fade(dx);
    double ty = fade(dy);
    double tz = fade(dz);

    double x1 = Lerp(g000, g100, tx);
    double x2 = Lerp(g010, g110, tx);
    double x3 = Lerp(g001, g101, tx);
    double x4 = Lerp(g011, g111, tx);

    double y1 = Lerp(x1, x2, ty);
    double y2 = Lerp(x3, x4, ty);

    return Lerp(y1, y2, tz);
}

double PerlinNoise3D::grad(int hash, double x, double y, double z) {
    hash &= 15;
    double v0 = hash < 8 ? x : y;
    double v1 = hash < 4 ? y : (hash == 12 || hash == 14) ? x : z;
    v0 = (hash & 1) == 0 ? v0 : -v0;
    v1 = (hash & 2) == 0 ? v1 : -v1;
    return v0 + v1;
}

static double fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}
