#include "simplex_noise.h"

#include <cmath>

static inline int fast_floor(float v) {
    int i = static_cast<int>(v);
    return (v < i) ? (i - 1) : (i);
}

SimplexNoise2D::SimplexNoise2D(std::mt19937& gen) {
    std::uniform_int_distribution<int32_t> dist(0, 7);
    // Randomize the hashes table.
    for (int i = 0; i < GRID_SIZE; i++) {
        m_hashes[i] = dist(gen);
    }
}

float SimplexNoise2D::sample(float x, float y) {

    // F and G both skew the standard basis grid into equilateral triangles and are
    // inverses factors of each other.
    static const float F = (sqrt(3) - 1) / 2;
    static const float G = (3 - sqrt(3)) / 6;

    // We will assume x,y lie in a lattice of equilateral triangles formed by:
    // f(p) = (p.x - (p.x + p.y)G, p.y - (p.x + p.y)G)
    //
    // This is chosen over the lattice formed by skewing by F since the lattice
    // formed by that does not have its triangles fully cover the positive x,y
    // plane.

    // Unskewing the lattice into a grid of squares so we may take the floor
    // in order to determine the triangle's indices.
    //
    // Appies a transition matrix given our input coordinates are using standard
    // basis.
    float s = (x + y) * F;
    int i = static_cast<int>(fast_floor(x + s));
    int j = static_cast<int>(fast_floor(y + s));

    // Computing the coordinates of the corner at i,j.
    // Coordinates are transforms back into the original cartesian coordinates
    // so euclidean distances can be calculated correctly.
    float q0 = (i + j) * G;
    float c0x = i - q0;
    float c0y = j - q0;

    // Computing distances from the corner at vertex i,j.
    float d0x = x - c0x;
    float d0y = y - c0y;

    // i,j+1  i+1,j+1
    //    |--/|
    //    | / |
    //    |/--|
    //  i,j   i+1,j
    int io, jo;
    if (d0x > d0y) { // Within lower triangle
        io = 1;
        jo = 0;
    } else { // Within upper triangle
        io = 0;
        jo = 1;
    }

    // Computing distances for the remaining two corners.
    float d1x = d0x - io + G;
    float d1y = d0y - jo + G;

    float d2x = d0x - 1.0f + 2.0f * G;
    float d2y = d0y - 1.0f + 2.0f * G;

    // Sample the grid of gradient hashes.
    // Making sure to wrap the coordinates to not go out of bounds.
    int i0 = wrap(i);
    int j0 = wrap(j);
    int i1 = wrap(i + io);
    int j1 = wrap(j + jo);
    int i2 = wrap(i + 1);
    int j2 = wrap(j + 1);
    int32_t h0 = m_hashes[j0*GRID_VECTOR_WIDTH_COUNT + i0];
    int32_t h1 = m_hashes[j1*GRID_VECTOR_WIDTH_COUNT + i1];
    int32_t h2 = m_hashes[j2*GRID_VECTOR_WIDTH_COUNT + i2];

    // Computing the contribution of each corner.
    auto contribute = [](int32_t hash, float dx, float dy) -> float {
        float dist = dx*dx + dy*dy;
        float t = 0.5f - dist;
        if (t < 0.0f) {
            return 0.0f;
        }
        return t * t * t * t * grad(hash, dx, dy);
    };

    float cn0 = contribute(h0, d0x, d0y);
    float cn1 = contribute(h1, d1x, d1y);
    float cn2 = contribute(h2, d2x, d2y);

    return 45.23065f * (cn0 + cn1 + cn2);
}

int SimplexNoise2D::wrap(int x) {
    return x & GRID_CELL_MASK;
}

float SimplexNoise2D::grad(int32_t hash, float x, float y) {
    int32_t h = hash & 0x7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}



SimplexNoise3D::SimplexNoise3D(std::mt19937& gen) {

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

float SimplexNoise3D::sample(float x, float y, float z) {

    // F and G both skew the standard basis grid into skewed tetrahedrons.
    static const float F = 1.0f / 3.0f;
    static const float G = 1.0f / 6.0f;

    // We will assume x,y,z lie in a lattice of skewed tetrahedrons formed by:
    // s = (p.x + p.y + p.z)G
    // f(p) = (p.x - s, p.y - s, p.z - s)
    //
    // This is chosen over the lattice formed by skewing by F since the lattice
    // formed by that does not have its tetrahedrons fully cover the positive x,y,z
    // plane.

    // Unskewing the lattice into a grid of squares so we may take the floor
    // in order to determine the triangle's indices.
    //
    // Appies a transition matrix given our input coordinates are using standard
    // basis.
    float s = (x + y + z) * F;
    int32_t i = static_cast<int32_t>(fast_floor(x + s));
    int32_t j = static_cast<int32_t>(fast_floor(y + s));
    int32_t k = static_cast<int32_t>(fast_floor(z + s));


    // Computing the coordinates of the corner at i,j,k.
    // Coordinates are transforms back into the original cartesian coordinates
    // so euclidean distances can be calculated correctly.
    float q0 = (i + j + k) * G;
    float c0x = i - q0;
    float c0y = j - q0;
    float c0z = k - q0;

    // Computing distances from the corner at vertex i,j,k.
    float d0x = x - c0x;
    float d0y = y - c0y;
    float d0z = z - c0z;

    // The skewed cube is formed from the skewing of 6 tetrahedrons
    // combined together. Determining the offsets to the second and
    // third vertices of the tetrahedron the point x,y,z lies in.
    int io1, jo1, ko1; // offset to the second vertex indices
    int io2, jo2, ko2; // offset to the third vertex indices
    if (d0y > d0x) {
        if (d0x > d0z) {        // y > x > z
            io1 = 0; jo1 = 1; ko1 = 0;
            io2 = 1; jo2 = 1; ko2 = 0;
        } else if (d0y > d0z) { // y > z > x
            io1 = 0; jo1 = 1; ko1 = 0;
            io2 = 0; jo2 = 1; ko2 = 1;
        } else {                // z > y > x
            io1 = 0; jo1 = 0; ko1 = 1;
            io2 = 0; jo2 = 1; ko2 = 1;
        }
    } else {
        if (d0y > d0z) {        // x > y > z
            io1 = 1; jo1 = 0; ko1 = 0;
            io2 = 1; jo2 = 1; ko2 = 0;
        } else if (d0x > d0z) { // x > z > y
            io1 = 1; jo1 = 0; ko1 = 0;
            io2 = 1; jo2 = 0; ko2 = 1;
        } else {                // z > x > y
            io1 = 0; jo1 = 0; ko1 = 1;
            io2 = 1; jo2 = 0; ko2 = 1;
        }
    }

    // The values here can be derived algebraic substitution with
    // a simplifying fact that io1,jo1,ko1 always adds to 1 and
    // io2,jo2,ko2 always adds to 2.
    float d1x = d0x - io1 + G;
    float d1y = d0y - jo1 + G;
    float d1z = d0z - ko1 + G;

    float d2x = d0x - io2 + 2.0f * G;
    float d2y = d0y - jo2 + 2.0f * G;
    float d2z = d0z - ko2 + 2.0f * G;

    float d3x = d0x - 1.0f + 3.0f * G;
    float d3y = d0y - 1.0f + 3.0f * G;
    float d3z = d0z - 1.0f + 3.0f * G;

    // Sample the grid of permutations to get a hash.
    // Making sure to wrap the coordinates to not go out of bounds.
    int32_t i0 = wrap(i);
    int32_t j0 = wrap(j);
    int32_t k0 = wrap(k);
    int32_t i1 = wrap(i + io1);
    int32_t j1 = wrap(j + jo1);
    int32_t k1 = wrap(k + ko1);
    int32_t i2 = wrap(i + io2);
    int32_t j2 = wrap(j + jo2);
    int32_t k2 = wrap(k + ko2);
    int32_t i3 = wrap(i + 1);
    int32_t j3 = wrap(j + 1);
    int32_t k3 = wrap(k + 1);
    int32_t h0 = perm(i0 + perm(j0 + perm(k0)));
    int32_t h1 = perm(i1 + perm(j1 + perm(k1)));
    int32_t h2 = perm(i2 + perm(j2 + perm(k2)));
    int32_t h3 = perm(i3 + perm(j3 + perm(k3)));

    // Computing the contribution of each corner.
    auto contribute = [](int32_t hash, float dx, float dy, float dz) -> float {
        float dist = dx*dx + dy*dy + dz*dz;
        float t = 0.6f - dist;
        if (t < 0.0f) {
            return 0.0f;
        }
        return t * t * t * t * grad(hash, dx, dy, dz);
    };

    float cn0 = contribute(h0, d0x, d0y, d0z);
    float cn1 = contribute(h1, d1x, d1y, d1z);
    float cn2 = contribute(h2, d2x, d2y, d2z);
    float cn3 = contribute(h3, d3x, d3y, d3z);

    return 32.0f * (cn0 + cn1 + cn2 + cn3);
}

int32_t SimplexNoise3D::wrap(int32_t x) {
    return x & 255;
}

int32_t SimplexNoise3D::perm(int32_t i) {
    return static_cast<int32_t>(m_perm[i]);
}

float SimplexNoise3D::grad(int32_t hash, float x, float y, float z) {
    hash &= 15;
    float v0 = hash < 8 ? x : y;
    float v1 = hash < 4 ? y : (hash == 12 || hash == 14) ? x : z;
    v0 = (hash & 1) == 0 ? v0 : -v0;
    v1 = (hash & 2) == 0 ? v1 : -v1;
    return v0 + v1;
}
