#pragma once

#include <cstdint>
#include <random>
#include <raylib.h>
#include <vector>

class Skybox {
public:

    Skybox(Shader shader, std::mt19937& gen);

    // Don't move or copy.
    // ---------------------
    Skybox(const Skybox&) = delete;
    Skybox(Skybox&&) = delete;
    Skybox& operator=(const Skybox&) = delete;
    Skybox& operator=(Skybox&&) = delete;

    void render();

    ~Skybox();

private:
    const Shader m_shader;

    TextureCubemap m_cubemap_texture;
    Model m_model;

    void initialize(std::mt19937& gen);

    static std::vector<uint32_t> create_side_texture(std::mt19937& gen);
};