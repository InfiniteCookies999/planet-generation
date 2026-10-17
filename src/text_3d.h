#pragma once

#include <raylib.h>

// Renders a UTF-8 encoded string of characters out in 3D.
//
// taken from example:
// https://www.raylib.com/examples/text/loader.html?name=text_3d_drawing
void draw_text_3d(Font font, const char* text, Vector3 position, float font_size,
                  float line_spacing, float codepoint_spacing,
                  bool backface, Color tint);

// taken from example:
// https://www.raylib.com/examples/text/loader.html?name=text_3d_drawing
void draw_text_codepoint_3d(Font font, int codepoint, Vector3 position, float font_size,
                            bool backface, Color tint);
