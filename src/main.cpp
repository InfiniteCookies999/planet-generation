
#include "config.h"
#include "controlled_camera.h"
#include "geometry.h"
#include "perlin_noise.h"
#include "text_3d.h"
#include "icosphere.h"

#include <cmath>
#include <format>
#include <cstring>
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <string>
#include <vector>

static float degrees(float radians) {
    return radians * (180.f / PI);
}

int main() {

    InitWindow(1024, 700, "planet generation");

    SetTargetFPS(60);

    Shader lighting_shader = LoadShader("assets/shaders/light.vs", "assets/shaders/light.fs");
    Shader discard_alpha_shader = LoadShader(NULL, "assets/shaders/discard_zero_alpha.fs");
    Shader star_shader = LoadShader(NULL, "assets/shaders/star.fs");
    Texture perlin_hashes_texture;
    {

        std::random_device rd;
        std::mt19937 gen(rd());
        std::vector<int> hashes = PerlinNoise::get_random_gradiant_hashes(gen);

        std::vector<float> img_data;
        img_data.reserve(hashes.size());
        for (int hash : hashes) {
            img_data.push_back(hash / 255.0f);
        }

        Image img;
        img.format = PIXELFORMAT_UNCOMPRESSED_R32;
        img.width = hashes.size();
        img.height = 1; // TODO (maddie): may want to space out.
        img.mipmaps = 1;
        img.data = &img_data[0];
        perlin_hashes_texture = LoadTextureFromImage(img);
    }

    Texture earth_texture = LoadTexture("assets/textures/earth_2x1.png");

    ControlledCamera camera({ 0, 5, 20 }, { 0, 0, 0 }, 5.f, +30.f);

    int subdivisions = 5;

    Geometry planet_geom = create_icosphere_geometry(subdivisions, 3);

    Model icosphere_model = LoadModelFromMesh(create_icosphere_mesh(subdivisions, 8, false));
    icosphere_model.materials[0].shader = star_shader;
    icosphere_model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = perlin_hashes_texture;

    Model cube_model = LoadModelFromMesh(GenMeshCube(2, 2, 2));
    cube_model.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = perlin_hashes_texture;

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

        Ray ray = GetScreenToWorldRay(GetMousePosition(), camera.get_raylib_camera());

        float hit_distance = 0;
        const Triangle* hit_tri = nullptr;
        RayCollision nearest_collision = { 0 };
        for (const Triangle& tri : planet_geom.triangles) {
            Vector3 a = planet_geom.positions[tri.a_index];
            Vector3 b = planet_geom.positions[tri.b_index];
            Vector3 c = planet_geom.positions[tri.c_index];
            RayCollision collision = GetRayCollisionTriangle(ray, a, b, c);
            if (collision.hit) {
                if (!hit_tri || collision.distance < hit_distance) {
                    hit_tri = &tri;
                    hit_distance = collision.distance;
                    nearest_collision = collision;
                }
            }
        }


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

        DrawModel(icosphere_model, { 0 }, 1.f, RAYWHITE);
        // DrawModel(cube_model, { 0 }, 1.f, RAYWHITE);


        Vector3* nearest_point = nullptr;
        if (hit_tri) {

            Vector3 hit_point = nearest_collision.point;

            Vector3 a = planet_geom.positions[hit_tri->a_index];
            Vector3 b = planet_geom.positions[hit_tri->b_index];
            Vector3 c = planet_geom.positions[hit_tri->c_index];

            float a_dist = Vector3Distance(a, hit_point);
            float b_dist = Vector3Distance(b, hit_point);
            float c_dist = Vector3Distance(c, hit_point);

            if (a_dist < b_dist && a_dist < c_dist) {
                nearest_point = &a;
            } else if (b_dist < c_dist) {
                nearest_point = &b;
            } else {
                nearest_point = &c;
            }

            Vector3 ba = b - a;
            Vector3 ca = c - a;
            Vector3 normal = Vector3CrossProduct(ba, ca);

            // DrawCircle3D(nearest_point, 0.05f, normal, 0.0f, RED);
            DrawPoint3D(*nearest_point, RED);
            // DrawTriangle3D(a, b, c, RED);
        }

        DrawGrid(10, 1.0f);


        EndMode3D();
        // ------------------------

        if (nearest_point) {
            float longitude = atan2(nearest_point->z, nearest_point->x);
            if (longitude < 0) longitude += 2*PI;
            float length = Vector3Length(*nearest_point);
            float latitude = asin(nearest_point->y / length) + (0.5f*PI);

            longitude /= PI;
            latitude /= 2*PI;

            std::string latitude_text = std::format("latitude: {:.2f}", latitude);
            std::string longitude_text = std::format("longitude: {:.2f}", longitude);
            DrawTextEx(font_x16, latitude_text.c_str(), { 0, 0 }, 16.f, 0.f, WHITE);
            DrawTextEx(font_x16, longitude_text.c_str(), { 0, 16 }, 16.f, 0.f, WHITE);
        }

        // DrawTexture(perlin_hashes_texture, 0, 0, RAYWHITE);

        EndDrawing();

        ++tick;

    }

    UnloadFont(font_x64);
    UnloadShader(lighting_shader);
    UnloadShader(discard_alpha_shader);
    UnloadModel(icosphere_model);
    UnloadModel(cube_model);
    UnloadTexture(earth_texture);
    UnloadTexture(perlin_hashes_texture);
    CloseWindow();

    return 0;
}
