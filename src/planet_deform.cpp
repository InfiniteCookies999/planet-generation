#include "planet_deform.h"

#include "geometry.h"
#include "icosphere.h"
#include "raylib.h"

#include <cassert>
#include <cmath>
#include <raymath.h>


Geometry create_deformed_planet(std::mt19937& gen,float radius) {

    Geometry geom = create_icosphere_geometry(3, 3);

    //PerlinNoise perlin_noise(gen);
    for (Vector3& position : geom.positions) {
        float longitude = atan2(position.z, position.x);
        if (longitude < 0) longitude += 2*PI; // 0 to 2 PI

        float length = Vector3Length(position);
        float latitude = asin(position.y / length) + (0.5f*PI); // 0 to PI

        // normalizing into 0 to 1.
        longitude /= PI;
        latitude /= 2*PI;

        const int cell_coverage = 4;
        for (int x = 0; x < 100; x++) {
            float sx = (longitude / 100.0f) * cell_coverage; // 0 to 1
            float sy = 0;
            // perlin_noise.sample(sx / 2.0f, sy);
        }


    }


    return geom;
}
