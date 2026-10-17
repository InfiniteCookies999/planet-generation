#include "icosphere.h"

#include <cassert>
#include <raylib.h>
#include <raymath.h>
#include <unordered_map>
#include <vector>

static const float HALF_PHI = 0.809016994374945;

static const float q = HALF_PHI;
static const float l = 0.5;

// The 12 vertices on an icosahedron. Created from the 3 goldren rectangles
// that encompass the icosahedron. See wikipedia page for visual:
// https://en.wikipedia.org/wiki/Regular_icosahedron
static const Vector3 VERTEX_POINTS[12] = {
    { -q, 0, -l }, { -q, 0, l }, { q, 0, -l }, { q, 0, l }, // golden rectangle 1 (XZ)
    { -l, -q, 0 }, { l, -q, 0 }, { -l, q, 0 }, { l, q, 0 }, // golden rectangle 2 (XY)
    { 0, -l, -q }, { 0, l, -q }, { 0, -l, q }, { 0, l, q }, // golden rectangle 3 (YZ)
};

// The 20 triangles that make up an icosahedron.
static const int NUM_TRIANGLES = 20;
static const size_t TRIANGLE_INDICES[NUM_TRIANGLES][3] = {
    // // top 8 triangles
    { 0, 1, 6 }, { 0, 6, 9 }, { 1, 11, 6 },
    { 6, 7, 9 }, { 6, 11, 7 },
    { 2, 9, 7 }, { 3, 7, 11 }, { 2, 7, 3 },
    // 4 connecting triangles
    { 0, 9, 8 }, { 2, 8, 9 }, { 1, 10, 11 }, { 3, 11, 10 },
    // // bottom 8 triangle
    { 0, 4, 1 }, { 0, 8, 4 }, { 1, 4, 10 },
    { 4, 8, 5 }, { 4, 5, 10 },
    { 2, 5, 8 }, { 3, 10, 5 }, { 2, 3, 5 }
};

struct Triangle {
    size_t a_index;
    size_t b_index;
    size_t c_index;
};

class IcosphereGenerator {
public:

    IcosphereGenerator(int _subdivision_count, float _radius);

    Mesh generate();

private:
    // The 6 shared triangles for each vertex.
    struct MappedTriangles {
        Triangle triangles[6];
        int tri_count = 0;
    };

    const float radius;
    int subdivision_count;
    size_t total_triangles;

    std::vector<Triangle> triangles;
    std::vector<Triangle> triangles2; // swap for subdivision.
    std::vector<Vector3> positions;
    std::unordered_map<int64_t, size_t> cached_positions;
    std::unordered_map<size_t, MappedTriangles> mapped_triangles_lookup;
    std::unordered_map<size_t, Vector3> cached_normals;

    void gen_icosahedron_triangles();

    void subdivide(const Triangle& tri, std::vector<Triangle>& triangles);

    void map_vertices_to_triangles();

    size_t create_and_get_mid_point_index(size_t index0, size_t index1);

    Vector3 get_averaged_normal(size_t index);

    Vector3 project_to_sphere(Vector3 point) const;

};

static void put_flat_triangle(Mesh& mesh, int offset, Vector3 a, Vector3 b, Vector3 c);

static void put_triangle(Mesh& mesh, int offset, Vector3 a, Vector3 b, Vector3 c,
                         Vector3 a_normal, Vector3 b_normal, Vector3 c_normal);

Mesh create_perpendicular_golden_rectangle_mesh() {

    const int NUM_TRIANGLES = 6;

    Mesh mesh = { 0 };
    mesh.triangleCount = NUM_TRIANGLES;
    mesh.vertexCount = mesh.triangleCount * 3;
    // Not using new operator because UnloadMesh may rely on the implementation of MemAlloc.
    mesh.vertices = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));
    mesh.normals = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));

    const int triangle_indices[NUM_TRIANGLES][3] = {
        { 0, 2, 3   }, { 0, 3, 1  },
        { 4, 6, 7   }, { 4, 7, 5  },
        { 8, 10, 11 }, { 8, 11, 9 }
    };

    for (int t = 0; t < NUM_TRIANGLES; t++) {

        int i0 = triangle_indices[t][0];
        int i1 = triangle_indices[t][1];
        int i2 = triangle_indices[t][2];

        Vector3 a = VERTEX_POINTS[i0];
        Vector3 b = VERTEX_POINTS[i1];
        Vector3 c = VERTEX_POINTS[i2];

        int o = t * 3 * 3;
        put_flat_triangle(mesh, o, a, b, c);
    }

    // Send the memory to the GPU.
    UploadMesh(&mesh, false);

    return mesh;
}

Mesh create_icosahedron_mesh() {

    Mesh mesh = { 0 };
    mesh.triangleCount = NUM_TRIANGLES;
    mesh.vertexCount = mesh.triangleCount * 3;
    // Not using new operator because UnloadMesh may rely on the implementation of MemAlloc.
    mesh.vertices = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));
    mesh.normals = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));

    for (int t = 0; t < NUM_TRIANGLES; t++) {

        size_t i0 = TRIANGLE_INDICES[t][0];
        size_t i1 = TRIANGLE_INDICES[t][1];
        size_t i2 = TRIANGLE_INDICES[t][2];

        Vector3 a = VERTEX_POINTS[i0];
        Vector3 b = VERTEX_POINTS[i1];
        Vector3 c = VERTEX_POINTS[i2];

        int o = t * 3 * 3;
        put_flat_triangle(mesh, o, a, b, c);
    }

    // Send the memory to the GPU.
    UploadMesh(&mesh, false);

    return mesh;
}

Mesh create_icosphere_mesh(int subdivision_count, float radius) {
    IcosphereGenerator generator(subdivision_count, radius);
    return generator.generate();
}


IcosphereGenerator::IcosphereGenerator(int _subdivision_count, float _radius)
    : radius(_radius)
    , subdivision_count(_subdivision_count)
    , total_triangles(static_cast<size_t>(20 * (1 << (2 * subdivision_count))))
    {
    triangles.reserve(total_triangles);
    triangles2.reserve(total_triangles);
    positions.reserve(total_triangles * 3);
}

Mesh IcosphereGenerator::generate() {

    gen_icosahedron_triangles();

    // Subdivide the icosahedron to make it have a higher resolution.
    while (subdivision_count > 0) {

        triangles2.clear();
        for (Triangle& tri : triangles) {
            subdivide(tri, triangles2);
        }
        triangles = triangles2;

        --subdivision_count;
    }

    // Map the vertices to the triangles so that later we can average
    // the connecting triangle normals of each vertex to create smooth
    // shading.
    map_vertices_to_triangles();

    // Generate the mesh from the sub-divided positions.
    Mesh mesh = { 0 };
    mesh.triangleCount = total_triangles;
    mesh.vertexCount = mesh.triangleCount * 3;
    // Not using new operator because UnloadMesh may rely on the implementation of MemAlloc.
    mesh.vertices = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));
    mesh.normals = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));

    int t = 0;
    for (const Triangle& tri : triangles) {
        int o = t * 3 * 3;
        Vector3 a = positions[tri.a_index];
        Vector3 b = positions[tri.b_index];
        Vector3 c = positions[tri.c_index];

        // put_flat_triangle(mesh, o, a, b, c);

        Vector3 a_normal = get_averaged_normal(tri.a_index);
        Vector3 b_normal = get_averaged_normal(tri.b_index);
        Vector3 c_normal = get_averaged_normal(tri.c_index);

        put_triangle(mesh, o, a, b, c, a_normal, b_normal, c_normal);
        ++t;
    }

    // Send the memory to the GPU.
    UploadMesh(&mesh, false);

    return mesh;
}

void IcosphereGenerator::gen_icosahedron_triangles() {
    for (int i = 0; i < 12; i++) {
        Vector3 position = VERTEX_POINTS[i];
        size_t index = positions.size();

        position = project_to_sphere(position);
        positions.push_back(position);

        cached_positions.insert({ i, index });
    }

    for (int t = 0; t < NUM_TRIANGLES; t++) {
        size_t a_index = TRIANGLE_INDICES[t][0];
        size_t b_index = TRIANGLE_INDICES[t][1];
        size_t c_index = TRIANGLE_INDICES[t][2];
        triangles.push_back({ a_index, b_index, c_index });
    }
}

void IcosphereGenerator::subdivide(const Triangle& tri, std::vector<Triangle>& triangles) {
    size_t mid_ba = create_and_get_mid_point_index(tri.a_index, tri.b_index);
    size_t mid_ca = create_and_get_mid_point_index(tri.a_index, tri.c_index);
    size_t mid_cb = create_and_get_mid_point_index(tri.c_index, tri.b_index);

    Triangle t0 = { tri.a_index, mid_ba, mid_ca };
    Triangle t1 = { tri.b_index, mid_cb, mid_ba };
    Triangle t2 = { tri.c_index, mid_ca, mid_cb };
    Triangle t3 = { mid_ba, mid_cb, mid_ca      };

    triangles.push_back(t0);
    triangles.push_back(t1);
    triangles.push_back(t2);
    triangles.push_back(t3);
}

void IcosphereGenerator::map_vertices_to_triangles() {
    for (const Triangle& tri : triangles) {
        MappedTriangles& a_mapped = mapped_triangles_lookup[tri.a_index];
        a_mapped.triangles[a_mapped.tri_count++] = tri;

        MappedTriangles& b_mapped = mapped_triangles_lookup[tri.b_index];
        b_mapped.triangles[b_mapped.tri_count++] = tri;

        MappedTriangles& c_mapped = mapped_triangles_lookup[tri.c_index];
        c_mapped.triangles[c_mapped.tri_count++] = tri;
    }
}

size_t IcosphereGenerator::create_and_get_mid_point_index(size_t index0, size_t index1) {

    if (index0 < index1) {
        std::swap(index0, index1);
    }

    int64_t cache_index = (static_cast<int64_t>(index0) << 32ll) | static_cast<int64_t>(index1);
    auto it = cached_positions.find(cache_index);
    if (it != cached_positions.end()) {
        return it->second;
    }

    Vector3 pos0 = positions[index0];
    Vector3 pos1 = positions[index1];
    Vector3 mid_point = (pos0 + pos1) / 2.f;
    mid_point = project_to_sphere(mid_point);

    int new_position_index = positions.size();
    positions.push_back(mid_point);
    cached_positions.insert({ cache_index, new_position_index });

    return new_position_index;
}

Vector3 IcosphereGenerator::get_averaged_normal(size_t index) {
    auto it = cached_normals.find(index);
    if (it != cached_normals.end()) {
        return it->second;
    }

    Vector3 summed_normal = { 0 };
    const MappedTriangles& mapped_triangles = mapped_triangles_lookup[index];
    for (int i = 0; i < 6; i++) {
        Triangle tri = mapped_triangles.triangles[i];
        Vector3 a = positions[tri.a_index];
        Vector3 b = positions[tri.b_index];
        Vector3 c = positions[tri.c_index];

        Vector3 ba = b - a;
        Vector3 ca = c - a;
        Vector3 normal = Vector3CrossProduct(ba, ca);
        summed_normal += normal;
    }

    Vector3 avg_normal = summed_normal / 6.f;
    cached_normals.insert({ index, avg_normal });
    return avg_normal;
}

Vector3 IcosphereGenerator::project_to_sphere(Vector3 point) const {
    Vector3 norm = Vector3Normalize(point);
    return norm * radius;
}

static void put_flat_triangle(Mesh& mesh, int offset, Vector3 a, Vector3 b, Vector3 c) {

    Vector3 ba = b - a;
    Vector3 ca = c - a;
    Vector3 normal = Vector3CrossProduct(ba, ca);

    put_triangle(mesh, offset, a, b, c, normal, normal, normal);

}

static void put_triangle(Mesh& mesh, int offset, Vector3 a, Vector3 b, Vector3 c,
                         Vector3 a_normal, Vector3 b_normal, Vector3 c_normal) {

    int o = offset;
    mesh.vertices[o + 0] = a.x;
    mesh.vertices[o + 1] = a.y;
    mesh.vertices[o + 2] = a.z;
    mesh.vertices[o + 3] = b.x;
    mesh.vertices[o + 4] = b.y;
    mesh.vertices[o + 5] = b.z;
    mesh.vertices[o + 6] = c.x;
    mesh.vertices[o + 7] = c.y;
    mesh.vertices[o + 8] = c.z;
    mesh.normals[o + 0] = a_normal.x;
    mesh.normals[o + 1] = a_normal.y;
    mesh.normals[o + 2] = a_normal.z;
    mesh.normals[o + 3] = b_normal.x;
    mesh.normals[o + 4] = b_normal.y;
    mesh.normals[o + 5] = b_normal.z;
    mesh.normals[o + 6] = c_normal.x;
    mesh.normals[o + 7] = c_normal.y;
    mesh.normals[o + 8] = c_normal.z;
}
