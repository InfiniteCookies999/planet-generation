
#include "controlled_camera.h"
#include "star_renderer.h"
#include "text_3d.h"

#include <cstring>
#include <iostream>
#include <random>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>
#include <chrono>

static long long current_time_nanoseconds();
static double current_time_milliseconds();

static float degrees(float radians) {
    return radians * (180.f / PI);
}

static Texture generate_star_perlin_texture(float freq);

int main() {

    InitWindow(1024, 700, "planet generation");

    SetTargetFPS(60);

    Shader lighting_shader = LoadShader("assets/shaders/light.vs", "assets/shaders/light.fs");
    Shader discard_alpha_shader = LoadShader(NULL, "assets/shaders/discard_zero_alpha.fs");
    Shader star_shader = LoadShader("assets/shaders/star.vs", "assets/shaders/star.fs");

    ControlledCamera camera({ 0, 5, 20 }, { 0, 0, 0 }, 5.f, +30.f);

    std::random_device rd;
    std::mt19937 gen(rd());
    StarRenderer star_model(star_shader);
    star_model.initialize(gen);

    int u_tick_loc = GetShaderLocation(star_shader, "u_tick");
    if (u_tick_loc == -1) {
        std::cerr << "** Could not find uniform variable u_tick\n";
        return -1;
    }

    const char* characters = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHI\nJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklm\nnopqrstuvwxyz{|}~";
    std::vector<int> codepoints;
    for (int i = 0; i < TextLength(characters); i++) {
        codepoints.push_back(static_cast<int>(characters[i]));
    }
    Font font_x64 = LoadFontEx("assets/fonts/DejaVuSansMono.ttf", 64, &codepoints[0], 95);
    Font font_x16 = LoadFontEx("assets/fonts/DejaVuSansMono.ttf", 16, &codepoints[0], 95);

    int tick = 0;
    while (!WindowShouldClose()) {

        camera.update();

        BeginDrawing();

        ClearBackground(BLACK);


        // 3D render --------------
        BeginMode3D(camera.get_raylib_camera());

        { // Render coordinates text.
            BeginShaderMode(discard_alpha_shader);

            draw_text_3d(font_x64, "+X", Vector3{ +4, 0, 0 }, 0.5f,  0.5f, 0.0f, true, BLUE);
            draw_text_3d(font_x64, "-X", Vector3{ -5, 0, 0 }, 0.5f,  0.5f, 0.0f, true, BLUE);

            draw_text_3d(font_x64, "+Z", Vector3{ 0, 0, +4 }, 0.5f,  0.5f, 0.0f, true, BLUE);
            draw_text_3d(font_x64, "-Z", Vector3{ 0, 0, -5 }, 0.5f,  0.5f, 0.0f, true, BLUE);

            EndShaderMode();
        }


        SetShaderValue(star_shader, u_tick_loc, &tick, SHADER_UNIFORM_INT);

        if (IsKeyPressed(KEY_F1)) {
            long long st = current_time_milliseconds();
            star_model.randomize(gen);
            double took = current_time_milliseconds() - st;
            std::cout << std::format("{:.2f}", took) << "ms\n";
        }

        star_model.render(tick);
        // DrawModelWires(icosphere_wired_model, { 0 }, 1.0f, BLUE);

        DrawGrid(10, 1.0f);


        EndMode3D();
        // ------------------------

        EndDrawing();

        ++tick;

    }

    UnloadFont(font_x64);
    UnloadShader(lighting_shader);
    UnloadShader(discard_alpha_shader);
    UnloadShader(star_shader);
    CloseWindow();

    return 0;
}

static long long current_time_nanoseconds() {
    using namespace std::chrono;
    auto timepoint = steady_clock::now();
    return duration_cast<nanoseconds>(timepoint.time_since_epoch()).count();
}

static double current_time_milliseconds() {
    return current_time_nanoseconds() * 1e-6;
}