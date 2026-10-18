
#include "controlled_camera.h"
#include "geometry.h"
#include "text_3d.h"
#include "icosphere.h"

#include <cstring>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>

int main() {

    InitWindow(1024, 700, "planet generation");

    SetTargetFPS(60);

    Shader lighting_shader = LoadShader("assets/shaders/light.vs", "assets/shaders/light.fs");
    Shader discard_alpha_shader = LoadShader(NULL, "assets/shaders/discard_zero_alpha.fs");

    ControlledCamera camera({ 0, 5, 12 }, { 0, 0, 0 }, 5.f, +30.f);

    Geometry planet_geom = create_icosphere_geometry(3, 3);

    Model icosphere_model = LoadModelFromMesh(create_icosphere_mesh(3, 3, false));

    // Model model = LoadModelFromMesh(GenMeshCube(1, 1, 1));
    icosphere_model.materials[0].shader = lighting_shader;

    const char* characters = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHI\nJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklm\nnopqrstuvwxyz{|}~";
    std::vector<int> codepoints;
    for (int i = 0; i < TextLength(characters); i++) {
        codepoints.push_back(static_cast<int>(characters[i]));
    }
    Font font = LoadFontEx("assets/fonts/DejaVuSansMono.ttf", 64, &codepoints[0], 95);

    while (!WindowShouldClose()) {

        camera.update();

        BeginDrawing();

        ClearBackground(BLACK);


        // 3D render --------------
        BeginMode3D(camera.get_raylib_camera());

        { // Render coordinates text.
            BeginShaderMode(discard_alpha_shader);

            draw_text_3d(font, "+X", Vector3{ +4, 0, 0 }, 0.5f,  0.5f, 0.0f, true, BLUE);
            draw_text_3d(font, "-X", Vector3{ -5, 0, 0 }, 0.5f,  0.5f, 0.0f, true, BLUE);

            draw_text_3d(font, "+Z", Vector3{ 0, 0, +4 }, 0.5f,  0.5f, 0.0f, true, BLUE);
            draw_text_3d(font, "-Z", Vector3{ 0, 0, -5 }, 0.5f,  0.5f, 0.0f, true, BLUE);

            EndShaderMode();
        }


        DrawModel(icosphere_model, { 0 }, 1.f, RAYWHITE);

        DrawGrid(10, 1.0f);

        EndMode3D();
        // ------------------------

        EndDrawing();

    }

    UnloadFont(font);
    UnloadShader(lighting_shader);
    UnloadShader(discard_alpha_shader);
    CloseWindow();

    return 0;
}
