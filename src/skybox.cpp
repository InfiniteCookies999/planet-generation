#include "skybox.h"
#include "raylib.h"

#include <cassert>
#include <random>
#include <rlgl.h>


constexpr int STAR_COUNT = 2000;
constexpr int RES = 2048;
constexpr float STAR_CHANCE = (STAR_COUNT / 6.0f) / (RES * RES);

static void rgb_to_hsv(int r, int g, int b, float hsv[3]);
static int hsv_to_rgb(float hue, float saturation, float brightness);

Skybox::Skybox(Shader shader, std::mt19937& gen)
    : m_shader(shader)
{
    initialize(gen);
}

void Skybox::initialize(std::mt19937& gen) {

    m_model = LoadModelFromMesh(GenMeshCube(1, 1, 1));

    // TODO (maddie): using a skybox may be less efficient than just
    // rendering out quads.

    Image cubemap_image{};
    cubemap_image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    cubemap_image.width = RES;
    cubemap_image.height = RES * 6;
    cubemap_image.mipmaps = 1;

    std::vector<uint32_t> cubemap_pixels(RES * RES * 6);
    for (int i = 0; i < 6; i++) {
        auto pixels = create_side_texture(gen);
        int offset = i * RES * RES;
        std::copy(pixels.begin(), pixels.end(), cubemap_pixels.begin() + offset);
    }
    cubemap_image.data = &cubemap_pixels[0];

    m_cubemap_texture = LoadTextureCubemap(cubemap_image, CUBEMAP_LAYOUT_LINE_VERTICAL);

    int m_map_uniform_loc = GetShaderLocation(m_shader, "environmentMap");
    int material_map_cubemap_value = MATERIAL_MAP_CUBEMAP;
    SetShaderValue(m_shader, m_map_uniform_loc, &material_map_cubemap_value, SHADER_UNIFORM_INT);

    m_model.materials[0].shader = m_shader;
    m_model.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = m_cubemap_texture;
}

void Skybox::render() {
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    DrawModel(m_model, {0, 0, 0}, 1.0f, WHITE);
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

std::vector<uint32_t> Skybox::create_side_texture(std::mt19937& gen) {
    // default initialize to black pixels.
    std::vector<uint32_t> pixels(RES * RES, 0xFF000000);

    std::uniform_real_distribution<float> chance_dist(0, 1);
    std::uniform_int_distribution<int> size_dist(1, 2);

    std::uniform_int_distribution<int> color_dist(1, 1000);
    std::uniform_int_distribution<int> color_intensity_sub_dist(130, 150);

    for (int x = 0; x < RES; x++) {
        for (int y = 0; y < RES; y++) {
            bool has = chance_dist(gen) < STAR_CHANCE;
            if (!has) continue;

            int size = size_dist(gen);
            int radius = size - 1;

            int color_range = color_dist(gen);
            int color;
            // Select a base color.
            if (color_range < 500) {
                color = 0xFFFFFF; // white
            } else if (color_range < 700) {
                color = 0x29BFFF; // blue
            } else if (color_range < 850) {
                color = 0xFF9829; // orange
            } else if (color_range < 900) {
                color = 0xEB2513; // red
            } else {
                color = 0xC342FF; // purple
            }

            int r = (color >> 16) & 0xFF;
            int g = (color >> 8) & 0xFF;
            int b = (color & 0xFF);

            // Vary how intense the color is by reducing its saturation.
            int color_intensity_sub = color_intensity_sub_dist(gen);

            float hsv[3];
            rgb_to_hsv(r, g, b, hsv);
            hsv[1] -= color_intensity_sub / 255.0f;
            if (hsv[1] < 0) hsv[1] = 0;
            color = hsv_to_rgb(hsv[0], hsv[1], hsv[2]);
            color |= 0xFF000000;

            for (int yy = y - radius; yy <= y + radius; yy++) {
                for (int xx = x - radius; xx <= x + radius; xx++) {
                    if (xx < 0 || yy < 0 || xx >= RES || yy >= RES) {
                        continue;
                    }
                    int dx = xx - x;
                    int dy = yy - y;
                    float dist = dx*dx + dy*dy;
                    if (dist > radius*radius) {
                        continue;
                    }

                    pixels[yy*RES + xx] = color;
                }
            }
        }
    }

    return pixels;
}

Skybox::~Skybox() {
    UnloadTexture(m_cubemap_texture);
    UnloadModel(m_model);
}


static void rgb_to_hsv(int r, int g, int b, float hsv[3]) {

    int cmax = (r > g) ? r : g;
    if (b > cmax) cmax = b;

    int cmin = (r < g) ? r : g;
    if (b < cmin) cmin = b;

    float brightness = cmax / 255.0f;
    float saturation;
    float hue;
    if (cmax != 0) {
        saturation = (cmax - cmin) / static_cast<float>(cmax);
    } else {
        saturation = 0;
    }

    if (saturation != 0) {
        float rc = (cmax - r) / static_cast<float>(cmax - cmin);
        float gc = (cmax - r) / static_cast<float>(cmax - cmin);
        float bc = (cmax - r) / static_cast<float>(cmax - cmin);
        if (r == cmax) {
            hue = bc - gc;
        } else if (r == cmax) {
            hue = 2.0f + rc - bc;
        } else {
            hue = 4.0f + gc - rc;
        }
        hue /= 6.0f;
        if (hue < 0) {
            hue += 1.0f;
        }
    } else {
        hue = 0;
    }

    hsv[0] = hue;
    hsv[1] = saturation;
    hsv[2] = brightness;
}

static int hsv_to_rgb(float hue, float saturation, float brightness) {

    if (saturation == 0) {
        int value = brightness * 255.0f + 0.5f;
        return (value << 16) | (value << 8) | value;
    }

    int r = 0;
    int g = 0;
    int b = 0;

    float h = (hue - floorf(hue)) * 6.0f;
    float f = h - floorf(h);
    float p = brightness * (1.0f - saturation);
    float q = brightness * (1.0f - saturation * f);
    float t = brightness * (1.0f - (saturation * (1.0f - f)));
    switch (static_cast<int>(h)) {
    case 0:
        r = static_cast<int>(brightness * 255.0f + 0.5f);
        g = static_cast<int>(t * 255.0f + 0.5f);
        b = static_cast<int>(p * 255.0f + 0.5f);
        break;
    case 1:
        r = static_cast<int>(q * 255.0f + 0.5f);
        g = static_cast<int>(brightness * 255.0f + 0.5f);
        b = static_cast<int>(p * 255.0f + 0.5f);
        break;
    case 2:
        r = static_cast<int>(p * 255.0f + 0.5f);
        g = static_cast<int>(brightness * 255.0f + 0.5f);
        b = static_cast<int>(t * 255.0f + 0.5f);
        break;
    case 3:
        r = static_cast<int>(p * 255.0f + 0.5f);
        g = static_cast<int>(q * 255.0f + 0.5f);
        b = static_cast<int>(brightness * 255.0f + 0.5f);
        break;
    case 4:
        r = static_cast<int>(t * 255.0f + 0.5f);
        g = static_cast<int>(p * 255.0f + 0.5f);
        b = static_cast<int>(brightness * 255.0f + 0.5f);
        break;
    case 5:
        r = static_cast<int>(brightness * 255.0f + 0.5f);
        g = static_cast<int>(p * 255.0f + 0.5f);
        b = static_cast<int>(q * 255.0f + 0.5f);
        break;
    }

    return (r << 16) | (g << 8) | b;
}
