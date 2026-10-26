#pragma once

#include <random>
#include <raylib.h>
#include <vector>

class StarRenderer {
public:

    StarRenderer(Shader star_shader);

    // Don't move or copy.
    // ---------------------

    StarRenderer(const StarRenderer&) = delete;
    StarRenderer(StarRenderer&&) = delete;

    StarRenderer& operator=(const StarRenderer&) = delete;
    StarRenderer& operator=(StarRenderer&&) = delete;

    void initialize(std::mt19937& gen);

    void render(int tick) const;

    void randomize(std::mt19937& gen);

    ~StarRenderer();

private:
    static const size_t TEXTURE_RESOLUTION = 1024;

    const Shader m_shader;

    bool m_initialized;
    Model m_model;

    std::vector<float> m_pixels;
    Texture m_texture0;
    Texture m_texture1;

    int m_tick_uniform_loc;
    int m_dark_color_uniform_loc;
    int m_dark_primary_color_uniform_loc;
    int m_bright_primary_color_uniform_loc;
    int m_brighter_color_uniform_loc;
    int m_white_color_uniform_loc;

    void set_shader_color(int uniform_loc, int hex_color) const;

    Texture generate_empty_texture();

    void generate_perlin_pixels(std::mt19937& gen, float freq);

};
