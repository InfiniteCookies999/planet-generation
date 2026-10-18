#include "perlin_noise.h"

#include <cmath>
#include <random>
#include <raymath.h>

static double grad(int hash, double x, double y);

static double fade(double t);

PerlinNoise::PerlinNoise(std::mt19937& gen) {
    std::uniform_int_distribution<> dist(0, 3);
    // Randomize the hashes table.
    for (int i = 0; i < GRID_SIZE; i++) {
        hashes[i] = dist(gen);
    }
}

double PerlinNoise::sample(double x, double y) {

    // Find the cell that the point x,y lies in.
    int cellX = static_cast<int>(floor(x)) & GRID_CELL_MASK;
    int cellY = static_cast<int>(floor(y)) & GRID_CELL_MASK;

    // Sample the grid of gradient hashes.
    int gbl = hashes[cellY*GRID_VECTOR_WIDTH_COUNT + cellX];
    int gbr = hashes[cellY*GRID_VECTOR_WIDTH_COUNT + (cellX+1)];
    int gtl = hashes[(cellY+1)*GRID_VECTOR_WIDTH_COUNT + cellX];
    int gtr = hashes[(cellY+1)*GRID_VECTOR_WIDTH_COUNT + (cellX+1)];

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

static double grad(int hash, double x, double y) {
    if      (hash == 0) return x;  // dot product with ( 1,  0)
    else if (hash == 1) return -x; // dot product with (-1,  0)
    else if (hash == 2) return y;  // dot product with ( 0,  1)
    else                return -y; // dot product with ( 0, -1)
}

static double fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}
