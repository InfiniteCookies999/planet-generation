#include "planet_deform.h"

#include "geometry.h"
#include "icosphere.h"
#include "perlin_noise.h"

#include <cmath>
#include <raymath.h>

Geometry create_deformed_planet(std::mt19937& gen,float radius) {

    Geometry geom = create_icosphere_geometry(3, 3);

    PerlinNoise perlin_noise(gen);
    for (Vector3& position : geom.positions) {
        float latitude = atan2(position.z, position.x);
        float longitude = asin(position.y);
    }


    // for (const Triangle& tri : geom.triangles) {
    //     Vector3 a = geom.positions[tri.a_index];
    //     Vector3 b = geom.positions[tri.b_index];
    //     Vector3 c = geom.positions[tri.c_index];
    // }


    return geom;
}
