#pragma once

#include "geometry.h"

Mesh create_perpendicular_golden_rectangle_mesh();

Mesh create_icosahedron_mesh();

Geometry create_icosphere_geometry(int subdivision_count, float radius);

Mesh create_icosphere_mesh(int subdivision_count, float radius, bool smooth_normals = true);
