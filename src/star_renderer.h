#pragma once

#include "controlled_camera.h"
#include "star.h"

#include <random>
#include <raylib.h>
#include <vector>

class StarRenderer {
public:

    StarRenderer(Shader star_shader, Shader star_rays_shader);

    // Don't move or copy.
    // ---------------------
    StarRenderer(const StarRenderer&) = delete;
    StarRenderer(StarRenderer&&) = delete;
    StarRenderer& operator=(const StarRenderer&) = delete;
    StarRenderer& operator=(StarRenderer&&) = delete;

    void prepare_render(const Star& star, std::mt19937& gen);

    void render(const Star& star, const ControlledCamera& camera, int tick) const;

    ~StarRenderer();

private:
    static const int TEXTURE_RESOLUTION = 1024;

    const Shader m_shader;
    const Shader m_rays_shader;

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

    Model create_model() const;

    void render_rays(const Star& star, const ControlledCamera& camera, int tick) const;

    void set_star_shader_color(int uniform_loc, int hex_color) const;
    static void set_shader_color(Shader shader, int uniform_loc, int hex_color);

    Texture generate_empty_texture();

    void create_geo_location_cache();

    void generate_perlin_pixels(std::mt19937& gen, float freq);

};
