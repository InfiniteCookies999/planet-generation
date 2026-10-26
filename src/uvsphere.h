#pragma once

#include "geometry.h"

#include <raylib.h>

/// @param segments The number of meridians containing vertices. Controls the resolution
///                 horizontally.
/// @param rings The number of parallels containing vertices. Controls the resolution
///              vertically.
///
Geometry create_uvsphere_geometry(int segments, int rings, float radius);

/// @param segments The number of meridians containing vertices. Controls the resolution
///                 horizontally.
/// @param rings The number of parallels containing vertices. Controls the resolution
///              vertically.
///
Mesh create_uvsphere_mesh(int segments, int rings, float radius, bool smooth_normals = true);
