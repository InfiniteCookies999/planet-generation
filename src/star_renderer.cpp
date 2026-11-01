#include "star_renderer.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <future>
#include <random>
#include <raylib.h>
#include <rlgl.h>

#include "controlled_camera.h"
#include "perlin_noise.h"
#include "raymath.h"
#include "uvsphere.h"


StarRenderer::StarRenderer(Shader star_shader, Shader star_ray_shader)
    : m_shader(star_shader)
    , m_ray_shader(star_ray_shader)
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

    m_model = LoadModelFromMesh(create_uvsphere_mesh(32, 64, m_radius, false));

    m_texture0 = generate_empty_texture();
    m_texture1 = generate_empty_texture();

    m_model.materials[0].shader = m_shader;
    m_model.materials[0].maps[MATERIAL_MAP_ALBEDO  ].texture = m_texture0;
    m_model.materials[0].maps[MATERIAL_MAP_ALBEDO+1].texture = m_texture1;

    m_star_tick_uniform_loc            = GetShaderLocation(m_shader, "u_tick");
    m_dark_color_uniform_loc           = GetShaderLocation(m_shader, "u_dark_color");
    m_dark_primary_color_uniform_loc   = GetShaderLocation(m_shader, "u_dark_primary_color");
    m_bright_primary_color_uniform_loc = GetShaderLocation(m_shader, "u_bright_primary_color");
    m_brighter_color_uniform_loc       = GetShaderLocation(m_shader, "u_brighter_color");
    m_white_color_uniform_loc          = GetShaderLocation(m_shader, "u_white_color");


    // m_ray_radius_uniform_loc     = GetShaderLocation(m_ray_shader, "u_ray_radius");
    m_ray_length_uniform_loc      = GetShaderLocation(m_ray_shader, "u_rays_length");
    m_star_radius_uniform_loc     = GetShaderLocation(m_ray_shader, "u_star_radius");
    m_ray_seed_uniform_loc        = GetShaderLocation(m_ray_shader, "u_seed");
    m_ray_color_near_uniform_loc  = GetShaderLocation(m_ray_shader, "u_ray_color_near");
    m_ray_color_far_uniform_loc   = GetShaderLocation(m_ray_shader, "u_ray_color_far");
    m_ray_tick_uniform_loc        = GetShaderLocation(m_ray_shader, "u_tick");
    m_star_center_uniform_loc     = GetShaderLocation(m_ray_shader, "u_star_center");

    create_geo_location_cache();

    randomize(gen);
}

void StarRenderer::render(const ControlledCamera& camera, int tick) const {
    assert(m_initialized && "Must call initialize() first");

    Vector3 center = { 0, 0, 0 };
    render_rays(center, camera, tick);

    SetShaderValue(m_shader, m_star_tick_uniform_loc, &tick, SHADER_UNIFORM_INT);
    DrawModel(m_model, center, 1.0f, WHITE);

}

void StarRenderer::randomize(std::mt19937& gen) {

    enum StarColors {
        C_WHITE, C_ORANGE, C_YELLOW, C_RED, C_LAST
    };

    std::uniform_int_distribution<> color_dist(0, C_LAST - 1);

    switch (color_dist(gen)) {
    case C_WHITE: {
        set_star_shader_color(m_dark_color_uniform_loc, 0xa9ded9);          // dark blue
        set_star_shader_color(m_dark_primary_color_uniform_loc, 0xc2f2ee);
        set_star_shader_color(m_bright_primary_color_uniform_loc, 0xedfffe);
        set_star_shader_color(m_brighter_color_uniform_loc, 0xf0fffe);
        set_star_shader_color(m_white_color_uniform_loc, 0xfaffff);
        m_ray_color_far = 0xc2f2ee;
        m_ray_color_near = 0xf0fffe;
        break;
    }
    case C_ORANGE: {
        set_star_shader_color(m_dark_color_uniform_loc, 0xc71b04);           // dark red
        set_star_shader_color(m_dark_primary_color_uniform_loc, 0xe02500);   // red
        set_star_shader_color(m_bright_primary_color_uniform_loc, 0xf28211); // orange
        set_star_shader_color(m_brighter_color_uniform_loc, 0xff9d2e);       // yellow
        set_star_shader_color(m_white_color_uniform_loc, 0xffe1a6);          // white
        m_ray_color_far = 0xe02500;
        m_ray_color_near = 0xff9d2e;
        break;
    }
    case C_YELLOW: {
        set_star_shader_color(m_dark_color_uniform_loc, 0xcf5702);           // dark red-orange
        set_star_shader_color(m_dark_primary_color_uniform_loc, 0xc95d04);   // orange
        set_star_shader_color(m_bright_primary_color_uniform_loc, 0xf5a12c); // yellow
        set_star_shader_color(m_brighter_color_uniform_loc, 0xffe369);       // bright yellow
        set_star_shader_color(m_white_color_uniform_loc, 0xffec9c);          // white
        m_ray_color_far = 0xc95d04;
        m_ray_color_near = 0xffe369;
        break;
    }
    case C_RED: {
        set_star_shader_color(m_dark_color_uniform_loc, 0xa61400); // dark red
        set_star_shader_color(m_dark_primary_color_uniform_loc, 0xc21b04); // red
        set_star_shader_color(m_bright_primary_color_uniform_loc, 0xf23005); // slightly less red
        set_star_shader_color(m_brighter_color_uniform_loc, 0xfa5320); // somewhat orange
        set_star_shader_color(m_white_color_uniform_loc, 0xff9457);
        m_ray_color_far = 0xc21b04;
        m_ray_color_near = 0xfa5320;
        break;
    }
    };

    std::uniform_real_distribution<float> freq0_dist(25, 50);
    std::uniform_real_distribution<float> freq1_dist(55, 80);

    generate_perlin_pixels(gen, freq0_dist(gen));
    UpdateTexture(m_texture0, &m_pixels[0]);

    generate_perlin_pixels(gen, freq1_dist(gen));
    UpdateTexture(m_texture1, &m_pixels[0]);

    std::uniform_int_distribution<unsigned> ray_seed_dist;
    m_rays_seed = ray_seed_dist(gen);

    // 1.2 is what i had it at originally.
    std::uniform_real_distribution<float> ray_radius_dist(1.0f, 1.4f);
    m_rays_length = ray_radius_dist(gen);

}

void StarRenderer::render_rays(const Vector3& center, const ControlledCamera& camera, int tick) const {

    rlDisableBackfaceCulling();
    BeginShaderMode(m_ray_shader);

    SetShaderValue(m_ray_shader, m_star_center_uniform_loc, &center, RL_SHADER_UNIFORM_VEC3);
    SetShaderValue(m_ray_shader, m_ray_length_uniform_loc, &m_rays_length, RL_SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_ray_shader, m_star_radius_uniform_loc, &m_radius, RL_SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_ray_shader, m_ray_seed_uniform_loc, &m_rays_seed, RL_SHADER_UNIFORM_UINT);
    set_shader_color(m_ray_shader, m_ray_color_near_uniform_loc, m_ray_color_near);
    set_shader_color(m_ray_shader, m_ray_color_far_uniform_loc, m_ray_color_far); // test color 0x42f581
    SetShaderValue(m_ray_shader, m_ray_tick_uniform_loc, &tick, RL_SHADER_UNIFORM_INT);

    render_rays_mesh(center, camera);

    EndShaderMode();
    rlEnableBackfaceCulling();

}

void StarRenderer::render_rays_mesh(const Vector3& center, const ControlledCamera& camera) const {

    Camera rl_camera = camera.get_raylib_camera();
    Vector3 forward = Vector3Normalize(rl_camera.target - rl_camera.position);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, rl_camera.up));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));

    float plane_radius = m_radius + m_rays_length;

    rlBegin(RL_QUADS);

        plane_radius += 4.0;

        rlTexCoord2f(0, 0);
        rlVertex3f(
            center.x + (-right.x * plane_radius) + (-up.x * plane_radius),
            center.y + (-right.y * plane_radius) + (-up.y * plane_radius),
            center.z + (-right.z * plane_radius) + (-up.z * plane_radius)
        );
        rlTexCoord2f(1, 0);
        rlVertex3f(
            center.x + (+right.x * plane_radius) + (-up.x * plane_radius),
            center.y + (+right.y * plane_radius) + (-up.y * plane_radius),
            center.z + (+right.z * plane_radius) + (-up.z * plane_radius)
        );
        rlTexCoord2f(1, 1);
        rlVertex3f(
            center.x + (+right.x * plane_radius) + (+up.x * plane_radius),
            center.y + (+right.y * plane_radius) + (+up.y * plane_radius),
            center.z + (+right.z * plane_radius) + (+up.z * plane_radius)
        );
        rlTexCoord2f(0, 1);
        rlVertex3f(
            center.x + (-right.x * plane_radius) + (+up.x * plane_radius),
            center.y + (-right.y * plane_radius) + (+up.y * plane_radius),
            center.z + (-right.z * plane_radius) + (+up.z * plane_radius)
        );
    rlEnd();
}

void StarRenderer::set_star_shader_color(int uniform_loc, int hex_color) const {
    set_shader_color(m_shader, uniform_loc, hex_color);
}

void StarRenderer::set_shader_color(Shader shader, int uniform_loc, int hex_color) {
    float buffer[3];

    int r = (hex_color >> 16) & 0xFF;
    int g = (hex_color >> 8) & 0xFF;
    int b = hex_color & 0xFF;

    buffer[0] = r / 255.0f;
    buffer[1] = g / 255.0f;
    buffer[2] = b / 255.0f;

    SetShaderValue(shader, uniform_loc, &buffer, SHADER_UNIFORM_VEC3);
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

void StarRenderer::create_geo_location_cache() {
    m_geo_location_cache.resize(TEXTURE_RESOLUTION * TEXTURE_RESOLUTION);

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

            int index = ty*TEXTURE_RESOLUTION + tx;
            m_geo_location_cache[index] = Vector3{ x, y, z };
        }
    }

}

void StarRenderer::generate_perlin_pixels(std::mt19937& gen, float freq) {

    PerlinNoise3D noise(gen);

    const int total = TEXTURE_RESOLUTION * TEXTURE_RESOLUTION;

    unsigned num_threads = std::thread::hardware_concurrency();
    unsigned used_threads = num_threads / 2;
    if (used_threads == 0) { // hardware_concurrency() is a hint and may return 0.
        used_threads = 1;
    }

    // TODO (maddie): is used_threads is 1 maybe we should skip using the future entirely.
    int chunk_size = static_cast<int>(std::ceilf(total / static_cast<float>(used_threads)));

    std::vector<std::future<void>> futures;
    for (unsigned t = 0; t < used_threads; ++t) {
        int start = t * chunk_size;
        int end = std::min(start + chunk_size, total);

        if (start >= total) {
            break;
        }

        futures.push_back(std::async(std::launch::async, [&, start, end]() {
            for (int i = start; i < end; i++) {
                const Vector3& geo_location = m_geo_location_cache[i];

                float density = noise.sample(
                    geo_location.x * freq,
                    geo_location.y * freq,
                    geo_location.z * freq
                );

                density = (density + 1.0f) * 0.5f;

                m_pixels[i] = density;
            }
        }));
    }

    for (auto& future : futures) {
        future.get();
    }
}
