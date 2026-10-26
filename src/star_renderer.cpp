#include "star_renderer.h"

#include <assert.h>
#include <random>
#include <raylib.h>

#include "perlin_noise.h"
#include "uvsphere.h"

StarRenderer::StarRenderer(Shader star_shader)
    : m_shader(star_shader)
    , m_initialized(false)
    , m_pixels(TEXTURE_RESOLUTION * TEXTURE_RESOLUTION)
{
}

StarRenderer::~StarRenderer() {
    if (!m_initialized) {
        return;
    }
    UnloadModel(m_model);
    UnloadTexture(m_texture0);
    UnloadTexture(m_texture1);
}

void StarRenderer::initialize(std::mt19937& gen) {
    m_initialized = true;
    // TODO (maddie): The UV sphere seems like a clear winner in reducing the vertex count to achieve the
    //                results we want to see. 4K triangles at 32 segment/64 rings vs. 20K triangles in
    //                an icosphere at 5 subdivisions.
    //
    //                However, even in a UV sphere where the rings are at is important. We do not nes.
    //                need as many rings in the center as at the poles. We can probably optimize the
    //                number of triangles down even more by just reducing that.
    //
    //                It may be possibly to even further reduce by using smoothed normals.

    m_model = LoadModelFromMesh(create_uvsphere_mesh(32, 64, 4, false));

    m_texture0 = generate_empty_texture();
    m_texture1 = generate_empty_texture();

    m_model.materials[0].shader = m_shader;
    m_model.materials[0].maps[MATERIAL_MAP_ALBEDO  ].texture = m_texture0;
    m_model.materials[0].maps[MATERIAL_MAP_ALBEDO+1].texture = m_texture1;

    m_tick_uniform_loc                 = GetShaderLocation(m_shader, "u_tick");
    m_dark_color_uniform_loc           = GetShaderLocation(m_shader, "u_dark_color");
    m_dark_primary_color_uniform_loc   = GetShaderLocation(m_shader, "u_dark_primary_color");
    m_bright_primary_color_uniform_loc = GetShaderLocation(m_shader, "u_bright_primary_color");
    m_brighter_color_uniform_loc       = GetShaderLocation(m_shader, "u_brighter_color");
    m_white_color_uniform_loc          = GetShaderLocation(m_shader, "u_white_color");

    randomize(gen);
}

void StarRenderer::render(int tick) const {
    assert(m_initialized && "Must call initialize() first");
    SetShaderValue(m_shader, m_tick_uniform_loc, &tick, SHADER_UNIFORM_INT);
    DrawModel(m_model, { 0 }, 1.f, RAYWHITE);
}

void StarRenderer::randomize(std::mt19937& gen) {

    enum StarColors {
        C_WHITE, C_ORANGE, C_YELLOW, C_RED, C_LAST
    };

    std::uniform_int_distribution<> color_dist(0, C_LAST - 1);

    switch (color_dist(gen)) {
    case C_WHITE: {
        set_shader_color(m_dark_color_uniform_loc, 0xa9ded9);          // dark blue
        set_shader_color(m_dark_primary_color_uniform_loc, 0xc2f2ee);
        set_shader_color(m_bright_primary_color_uniform_loc, 0xedfffe);
        set_shader_color(m_brighter_color_uniform_loc, 0xf0fffe);
        set_shader_color(m_white_color_uniform_loc, 0xfaffff);
        break;
    }
    case C_ORANGE: {
        set_shader_color(m_dark_color_uniform_loc, 0xc71b04);           // dark red
        set_shader_color(m_dark_primary_color_uniform_loc, 0xe02500);   // red
        set_shader_color(m_bright_primary_color_uniform_loc, 0xf28211); // orange
        set_shader_color(m_brighter_color_uniform_loc, 0xff9d2e);       // yellow
        set_shader_color(m_white_color_uniform_loc, 0xffe1a6);          // white
        break;
    }
    case C_YELLOW: {
        set_shader_color(m_dark_color_uniform_loc, 0xcf5702);           // dark red-orange
        set_shader_color(m_dark_primary_color_uniform_loc, 0xc95d04);   // orange
        set_shader_color(m_bright_primary_color_uniform_loc, 0xf5a12c); // yellow
        set_shader_color(m_brighter_color_uniform_loc, 0xffe369);       // bright yellow
        set_shader_color(m_white_color_uniform_loc, 0xffec9c);          // white
        break;
    }
    case C_RED: {
        set_shader_color(m_dark_color_uniform_loc, 0xa61400); // dark red
        set_shader_color(m_dark_primary_color_uniform_loc, 0xc21b04); // red
        set_shader_color(m_bright_primary_color_uniform_loc, 0xf23005); // slightly less red
        set_shader_color(m_brighter_color_uniform_loc, 0xfa5320); // somewhat orange
        set_shader_color(m_white_color_uniform_loc, 0xff9457);
        break;
    }
    };

    std::uniform_real_distribution<float> freq0_dist(25, 50);
    std::uniform_real_distribution<float> freq1_dist(55, 80);

    // TODO (maddie): calculating these is rather slow. It is 200K iterations.
    // The sampling calculations are all mutually exclusive so multi-threading
    // could be used.

    generate_perlin_pixels(gen, freq0_dist(gen));
    UpdateTexture(m_texture0, &m_pixels[0]);

    generate_perlin_pixels(gen, freq1_dist(gen));
    UpdateTexture(m_texture1, &m_pixels[0]);

}

void StarRenderer::set_shader_color(int uniform_loc, int hex_color) const {
    float buffer[4];

    int r = (hex_color >> 16) & 0xFF;
    int g = (hex_color >> 8) & 0xFF;
    int b = hex_color & 0xFF;

    buffer[0] = r / 255.0f;
    buffer[1] = g / 255.0f;
    buffer[2] = b / 255.0f;
    buffer[3] = 1.0f;

    SetShaderValue(m_shader, uniform_loc, &buffer, SHADER_UNIFORM_VEC4);
}

Texture StarRenderer::generate_empty_texture() {

    Image img;
    img.format = PIXELFORMAT_UNCOMPRESSED_R32;
    img.width = TEXTURE_RESOLUTION;
    img.height = TEXTURE_RESOLUTION;
    img.mipmaps = 1;
    img.data = &m_pixels[0];

    return LoadTextureFromImage(img);
}

void StarRenderer::generate_perlin_pixels(std::mt19937& gen, float freq) {

    PerlinNoise3D noise(gen);

    for (int ty = 0; ty < TEXTURE_RESOLUTION; ty++) {

        float v = ty / static_cast<float>(TEXTURE_RESOLUTION);
        float latitude = (v * PI) - 0.5f*PI; // -1/2PI to +1/2PI.
        float y = (sin(latitude) + 1) * 0.5f;

        for (int tx = 0; tx < TEXTURE_RESOLUTION; tx++) {
            float u = tx / static_cast<float>(TEXTURE_RESOLUTION);

            float longitude = u * 2 * PI; // 0 to 2 PI

            float x = sin(longitude) * cos(latitude);
            float z = cos(longitude) * cos(latitude);

            x = (x + 1) * 0.5f;
            z = (z + 1) * 0.5f;

            float density = noise.sample(x * freq, y * freq, z * freq);
            density = (density + 1.0f) * 0.5f;

            m_pixels[ty*TEXTURE_RESOLUTION + tx] = density;
        }
    }
}
