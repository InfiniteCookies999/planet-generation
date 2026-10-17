#include "text_3d.h"
#include <raylib.h>
#include <rlgl.h>

void draw_text_3d(Font font, const char* text, Vector3 position, float font_size,
                  float line_spacing, float codepoint_spacing,
                  bool backface, Color tint) {

    int length = TextLength(text);

    float scale = font_size / static_cast<float>(font.baseSize);

    float text_offset_x = 0;
    float text_offset_y = 0;

    for (int i = 0; i < length;) {
        int codepoint_by_byte_count = 0;
        int codepoint = GetCodepoint(&text[i], &codepoint_by_byte_count);
        int index = GetGlyphIndex(font, codepoint);

        if (codepoint == 0x3f) { // invalid codepoint.
            codepoint_by_byte_count = 1;
        }

        if (codepoint == '\n') {
            text_offset_y += font_size + line_spacing;
            text_offset_x = 0;
        } else {
            if (codepoint != ' ' && codepoint != '\t') {
                Vector3 codepoint_position = {
                    position.x + text_offset_x, position.y, position.z + text_offset_y
                };
                draw_text_codepoint_3d(font, codepoint, codepoint_position, font_size, backface, tint);
            }

            int advance_x = font.glyphs[index].advanceX;
            if (advance_x == 0) {
                text_offset_x += static_cast<float>(font.recs[index].width) * scale + codepoint_spacing;
            } else {
                text_offset_x += static_cast<float>(advance_x) * scale + codepoint_spacing;
            }
        }

        i += codepoint_by_byte_count;
    }

}

void draw_text_codepoint_3d(Font font, int codepoint, Vector3 position, float font_size,
                            bool backface, Color tint) {

    // Character index position in sprite font.
    int index = GetGlyphIndex(font, codepoint);

    float scale = font_size / static_cast<float>(font.baseSize);

    // TODO (maddie): what is this even doing?
    position.x += static_cast<float>(font.glyphs[index].offsetX - font.glyphPadding) * scale;
    position.z += static_cast<float>(font.glyphs[index].offsetY - font.glyphPadding) * scale;

    Rectangle texture_box = font.recs[index];

    float width = static_cast<float>(texture_box.width + 2*font.glyphPadding)*scale;
    float height = static_cast<float>(texture_box.height + 2*font.glyphPadding)*scale;

    // will render the batch if exceeded.
    rlCheckRenderBatchLimit(4 + 4*(backface ? 1 : 0));

    rlSetTexture(font.texture.id);

    Rectangle src_box = {
        texture_box.x - static_cast<float>(font.glyphPadding),
        texture_box.y - static_cast<float>(font.glyphPadding),
        texture_box.width + 2*font.glyphPadding,
        texture_box.height + 2*font.glyphPadding
    };
    const float u0 = src_box.x / font.texture.width;
    const float v0 = src_box.y / font.texture.height;
    const float u1 = (src_box.x + src_box.width) / font.texture.width;
    const float v1 = (src_box.y + src_box.height) / font.texture.height;

    rlPushMatrix();
    {
        rlTranslatef(position.x, position.y, position.z);
        rlBegin(RL_QUADS);

        rlColor4ub(tint.r, tint.g, tint.b, tint.a);

        // Font face points UP.
        rlNormal3f(0, 1, 0);
        rlTexCoord2f(u0, v0); rlVertex3f(0    , 0, 0     );
        rlTexCoord2f(u0, v1); rlVertex3f(0    , 0, height);
        rlTexCoord2f(u1, v1); rlVertex3f(width, 0, height);
        rlTexCoord2f(u1, v0); rlVertex3f(width, 0, 0     );

        // Back face points DOWN.
        if (backface) {
            rlNormal3f(0, -1, 0);
            rlTexCoord2f(u0, v0); rlVertex3f(0    , 0, 0     );
            rlTexCoord2f(u1, v0); rlVertex3f(width, 0, 0     );
            rlTexCoord2f(u1, v1); rlVertex3f(width, 0, height);
            rlTexCoord2f(u0, v1); rlVertex3f(0    , 0, height);
        }

        rlEnd();
    }
    rlPopMatrix();

}
