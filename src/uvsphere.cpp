#include "uvsphere.h"
#include "geometry.h"
#include "raylib.h"

#include <cassert>
#include <raymath.h>
#include <cmath>
#include <vector>

static void gen_texture_coordinates(const Geometry& geom, Mesh& mesh, float radius);

static void put_uv_from_coordinates(Mesh& mesh, int offset, float longitude, float latitude);

Geometry create_uvsphere_geometry(int segments, int rings, float radius) {
    assert(rings > 2);
    assert(segments >= 3);

    // useful article
    // https://danielsieger.com/blog/2021/03/27/generating-spheres.html

    std::vector<Vector3> positions;
    std::vector<Triangle> triangles;

    positions.reserve((rings - 1) * segments + 2);
    triangles.reserve(((rings - 2) * segments * 2) + segments * 2);

    // Top/Bottom vertex.
    positions.push_back(Vector3{ 0, radius, 0 });
    positions.push_back(Vector3{ 0, -radius, 0 });

    auto segments_for_phi = [segments, radius](float phi, std::vector<Vector3>& positions) {
        for (int j = 0; j < segments; j++) {
            // angle on the X/Z axis.
            float theta = 2.0 * PI * (static_cast<float>(j) / segments);
            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);
            positions.push_back(Vector3{ x, y, z } * radius);
        }
    };

    // TODO (maddie): probably should not be hardcoded.
    // Two extra rings are generated
    const float C = 0.05f;

    float top_phi = PI * (C / rings);
    segments_for_phi(top_phi, positions);
    for (int i = 0; i < rings - 1; i++) {
        // angle on the Y axis (resulting cos will range -1 to +1)
        float phi = PI * (static_cast<float>(i + 1) / rings);
        segments_for_phi(phi, positions);
    }
    float bottom_phi = PI * ((static_cast<float>(rings - 1) + (1 - C)) / rings);
    segments_for_phi(bottom_phi, positions);


    // Add top/bottom triangles.
    for (int i = 0; i < segments; i++) {
        { // top triangle
            int b_index = i + 2;
            int c_index = (i + 1) % segments + 2;
            triangles.push_back(Triangle{ 0, static_cast<size_t>(c_index), static_cast<size_t>(b_index) });
        }
        { // bottom triangle
            int offset = segments * rings + 2;
            int b_index = i + offset;
            int c_index = (i + 1) % segments + offset;
            triangles.push_back(Triangle{ 1, static_cast<size_t>(b_index), static_cast<size_t>(c_index) });
        }
    }

    // Add quads per ring.
    for (int i = 0; i < rings; i++) {
        // offsets into the ring.
        int r0 = i * segments + 2;
        int r1 = (i + 1) * segments + 2;
        for (int j = 0; j < segments; j++) {
            int index0 = r0 + j;
            int index1 = r0 + (j + 1) % segments;
            int index2 = r1 + j;
            int index3 = r1 + (j + 1) % segments;
            triangles.push_back(Triangle{
                static_cast<size_t>(index0),
                static_cast<size_t>(index1),
                static_cast<size_t>(index2)
            });
            triangles.push_back(Triangle{
                static_cast<size_t>(index1),
                static_cast<size_t>(index3),
                static_cast<size_t>(index2)
            });
        }
    }

    return Geometry{
        positions,
        triangles
    };
}

Mesh create_uvsphere_mesh(int segments, int rings, float radius, bool smooth_normals) {

    Geometry geom = create_uvsphere_geometry(segments, rings, radius);
    Mesh mesh = mesh_from_geometry(geom, smooth_normals, false);

    gen_texture_coordinates(geom, mesh, radius);

    // Send the memory to the GPU.
    UploadMesh(&mesh, false);

    return mesh;
}


// TODO (maddie): this is essentially a copy of what exists in icosphere.cpp.
static void gen_texture_coordinates(const Geometry& geom, Mesh& mesh, float radius) {

    // Not using new operator because UnloadMesh may rely on the implementation of MemAlloc.
    mesh.texcoords = static_cast<float*>(MemAlloc(mesh.vertexCount * 2 * sizeof(float)));

    auto get_longitude = [](const Vector3& point) {
        float longitude = atan2(point.z, point.x);
        if (longitude < 0) longitude += 2*PI; // 0 to 2 PI
        return longitude;
    };

    auto get_latitude = [radius](const Vector3& point) {
        return asin(point.y / radius) + (0.5f*PI); // 0 to PI
    };

    int t = 0;
    for (const Triangle& tri : geom.triangles) {

        int o = t * 3 * 2;
        Vector3 a = geom.positions[tri.a_index];
        Vector3 b = geom.positions[tri.b_index];
        Vector3 c = geom.positions[tri.c_index];


        float a_longitude = get_longitude(a);
        float b_longitude = get_longitude(b);
        float c_longitude = get_longitude(c);

        float a_latitude = get_latitude(a);
        float b_latitude = get_latitude(b);
        float c_latitude = get_latitude(c);

        bool a_converges = tri.a_index == 0 || tri.a_index == 1;
        bool b_converges = tri.b_index == 0 || tri.b_index == 1;
        bool c_converges = tri.c_index == 0 || tri.c_index == 1;

        // For the triangles on the near 2 PI the final points lay at 0 but needs to
        // wrap around to 2 PI.
        constexpr float PI15 =  1.5f * PI;
        if (abs(a_longitude - b_longitude) > PI15 ||
            abs(a_longitude - c_longitude) > PI15 ||
            abs(b_longitude - c_longitude) > PI15) {

            if (!a_converges && FloatEquals(a_longitude, 0.0f)) {
                a_longitude = 2 * PI;
            }
            if (!b_converges && FloatEquals(b_longitude, 0.0f)) {
                b_longitude = 2 * PI;
            }
            if (!c_converges && FloatEquals(c_longitude, 0.0f)) {
                c_longitude = 2 * PI;
            }
        }

        if (a_converges) {
            a_longitude = 0;
        }
        if (b_converges) {
            b_longitude = 0;
        }
        if (c_converges) {
            c_longitude = 0;
        }

        put_uv_from_coordinates(mesh, o  , a_longitude, a_latitude);
        put_uv_from_coordinates(mesh, o+2, b_longitude, b_latitude);
        put_uv_from_coordinates(mesh, o+4, c_longitude, c_latitude);

        ++t;
    }
}

static void put_uv_from_coordinates(Mesh& mesh, int offset, float longitude, float latitude) {
    float u = 1.0f - (longitude / (2*PI));
    float v = 1.0f -  (latitude / PI); // (latitude / 4.606826f);
    mesh.texcoords[offset + 0] = u;
    mesh.texcoords[offset + 1] = v;
}
