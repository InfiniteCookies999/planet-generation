#pragma once

#include "controlled_camera.h"

#include <random>
#include <raylib.h>
#include <vector>

class StarRenderer {
public:

    StarRenderer(Shader star_shader, Shader star_ray_shader);

    // Don't move or copy.
    // ---------------------
    StarRenderer(const StarRenderer&) = delete;
    StarRenderer(StarRenderer&&) = delete;
    StarRenderer& operator=(const StarRenderer&) = delete;
    StarRenderer& operator=(StarRenderer&&) = delete;

    void initialize(std::mt19937& gen);

    void render(const ControlledCamera& camera, int tick) const;

    void randomize(std::mt19937& gen);

    ~StarRenderer();

private:
    static const int TEXTURE_RESOLUTION = 1024;

    const Shader m_shader;
    const Shader m_ray_shader;
    const float m_radius = 4;

    bool m_initialized;
    Model m_model;

    std::vector<float> m_pixels;
    Texture m_texture0;
    Texture m_texture1;

    std::vector<Vector3> m_geo_location_cache;

    unsigned m_rays_seed;
    int m_ray_color_near;
    int m_ray_color_far;
    float m_rays_length;

    // star shader uniforms
    int m_star_tick_uniform_loc;
    int m_dark_color_uniform_loc;
    int m_dark_primary_color_uniform_loc;
    int m_bright_primary_color_uniform_loc;
    int m_brighter_color_uniform_loc;
    int m_white_color_uniform_loc;

    // ray shader uniforms
    int m_star_center_uniform_loc;
    // int m_ray_radius_uniform_loc;
    int m_ray_length_uniform_loc;
    int m_star_radius_uniform_loc;
    int m_ray_seed_uniform_loc;
    int m_ray_color_near_uniform_loc;
    int m_ray_color_far_uniform_loc;
    int m_ray_tick_uniform_loc;

    void render_rays(const Vector3& center, const ControlledCamera& camera, int tick) const;
    void render_rays_mesh(const Vector3& center, const ControlledCamera& camera) const;

    void set_star_shader_color(int uniform_loc, int hex_color) const;
    static void set_shader_color(Shader shader, int uniform_loc, int hex_color);

    Texture generate_empty_texture();

    void create_geo_location_cache();

    void generate_perlin_pixels(std::mt19937& gen, float freq);

};
