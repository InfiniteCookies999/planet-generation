#pragma once

#include <cstdint>
#include <random>
#include <raylib.h>
#include <vector>

class Skybox {
public:

    Skybox(Shader shader);

    // Don't move or copy.
    // ---------------------
    Skybox(const Skybox&) = delete;
    Skybox(Skybox&&) = delete;
    Skybox& operator=(const Skybox&) = delete;
    Skybox& operator=(Skybox&&) = delete;

    void initialize(std::mt19937& gen);

    void render();

    ~Skybox();

private:
    const Shader m_shader;

    bool m_initialized;

    TextureCubemap m_cubemap_texture;
    Model m_model;

    static std::vector<uint32_t> create_side_texture(std::mt19937& gen);
};