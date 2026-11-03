#pragma once

#include "controlled_camera.h"
#include <raylib.h>

/**
 * Draws a billboard plane without a texture. Raylib provides
 * DrawBillboard if a texture is expected. Texture coordinates
 * are still rendered.
 */
void render_billboard(const ControlledCamera& camera, Vector3 center, float half_size);
