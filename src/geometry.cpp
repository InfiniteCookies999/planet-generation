#include "geometry.h"

#include <raylib.h>
#include <raymath.h>
#include <unordered_map>

using MappedTriangles = std::unordered_map<size_t, std::vector<const Triangle*>>;

static Vector3 get_averaged_normal(const Geometry& geom,
                                   const MappedTriangles& mapped_triangles,
                                   std::unordered_map<size_t, Vector3>& cached_normals,
                                   size_t index);

Mesh mesh_from_geometry(const Geometry& geom, bool smooth_normals, bool upload_to_gpu) {

    // Generate the mesh from the sub-divided positions.
    Mesh mesh = { 0 };
    mesh.triangleCount = geom.triangles.size();
    mesh.vertexCount = mesh.triangleCount * 3;
    // Not using new operator because UnloadMesh may rely on the implementation of MemAlloc.
    mesh.vertices = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));
    mesh.normals = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));


    MappedTriangles mapped_triangles;
    std::unordered_map<size_t, Vector3> cached_normals;
    if (smooth_normals) {
        // Find all the triangles sharing the given vertex index.
        for (const Triangle& tri : geom.triangles) {
            mapped_triangles[tri.a_index].push_back(&tri);
            mapped_triangles[tri.b_index].push_back(&tri);
            mapped_triangles[tri.c_index].push_back(&tri);
        }
    }

    int t = 0;
    for (const Triangle& tri : geom.triangles) {
        int o = t * 3 * 3;
        Vector3 a = geom.positions[tri.a_index];
        Vector3 b = geom.positions[tri.b_index];
        Vector3 c = geom.positions[tri.c_index];

        if (smooth_normals) {
            Vector3 a_normal = get_averaged_normal(geom, mapped_triangles, cached_normals, tri.a_index);
            Vector3 b_normal = get_averaged_normal(geom, mapped_triangles, cached_normals, tri.b_index);
            Vector3 c_normal = get_averaged_normal(geom, mapped_triangles, cached_normals, tri.c_index);
            put_triangle(mesh, o, a, b, c, a_normal, b_normal, c_normal);
        } else {
            put_flat_triangle(mesh, o, a, b, c);
        }

        ++t;
    }

    // Send the memory to the GPU.
    if (upload_to_gpu) {
        UploadMesh(&mesh, false);
    }

    return mesh;
}

static Vector3 get_averaged_normal(const Geometry& geom,
                                   const MappedTriangles& mapped_triangles,
                                   std::unordered_map<size_t, Vector3>& cached_normals,
                                   size_t index) {
    auto it = cached_normals.find(index);
    if (it != cached_normals.end()) {
        return it->second;
    }

    const std::vector<const Triangle*>& triangles = mapped_triangles.find(index)->second;
    Vector3 summed_normal = { 0 };
    for (const Triangle* tri : triangles) {
        Vector3 a = geom.positions[tri->a_index];
        Vector3 b = geom.positions[tri->b_index];
        Vector3 c = geom.positions[tri->c_index];

        Vector3 ba = b - a;
        Vector3 ca = c - a;
        Vector3 normal = Vector3CrossProduct(ba, ca);
        summed_normal += normal;
    }

    Vector3 avg_normal = summed_normal / static_cast<float>(triangles.size());
    cached_normals.insert({ index, avg_normal });
    return avg_normal;
}

void put_flat_triangle(Mesh& mesh, int offset, Vector3 a, Vector3 b, Vector3 c) {
    Vector3 ba = b - a;
    Vector3 ca = c - a;
    Vector3 normal = Vector3CrossProduct(ba, ca);

    put_triangle(mesh, offset, a, b, c, normal, normal, normal);
}

void put_triangle(Mesh& mesh, int offset, Vector3 a, Vector3 b, Vector3 c,
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