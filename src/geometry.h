#pragma once

#include <cstddef>
#include <raylib.h>
#include <unordered_map>
#include <vector>

struct Triangle {
    size_t a_index;
    size_t b_index;
    size_t c_index;
};

struct Geometry {
    std::vector<Vector3> positions;
    std::vector<Triangle> triangles;
};

Mesh mesh_from_geometry(const Geometry& geom, bool smooth_lighting);

void put_flat_triangle(Mesh& mesh, int offset, Vector3 a, Vector3 b, Vector3 c);

void put_triangle(Mesh& mesh, int offset, Vector3 a, Vector3 b, Vector3 c,
                  Vector3 a_normal, Vector3 b_normal, Vector3 c_normal);
