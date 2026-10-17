
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

    // possible options for generating the mesh:
    // 1. Make a cube and then deform it. This has the downside that the corners
    //    may wind up wrong.
    // 2. generate an icosahedron then subdivide many times (seems to be an option in blender).
    // 3. UV sphere? seems not great since its not uniformly consistent.


    // I am thinking we try option 2.

    // can start with generating the most basic icosahedron without any subdivisions.

    Shader lighting_shader = LoadShader("assets/shaders/light.vs", "assets/shaders/light.fs");
    Shader discard_alpha_shader = LoadShader(NULL, "assets/shaders/discard_zero_alpha.fs");

    Camera camera = {
        .position = { 0, 3, 5 },
        .target = { 0, 0, 0 },
        .up = { 0, 1, 0 },
        .fovy = 45.f,
        .projection = CAMERA_PERSPECTIVE
    };

    Model rectangles_model = LoadModelFromMesh(create_perpendicular_golden_rectangle_mesh());
    Model icosahedron_model = LoadModelFromMesh(create_icosahedron_mesh());
    Model icosphere_model = LoadModelFromMesh(create_icosphere_mesh(3, 3));

    // Model model = LoadModelFromMesh(GenMeshCube(1, 1, 1));
    rectangles_model.materials[0].shader = lighting_shader;
    icosahedron_model.materials[0].shader = lighting_shader;
    icosphere_model.materials[0].shader = lighting_shader;

    const char* characters = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHI\nJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklm\nnopqrstuvwxyz{|}~";
    std::vector<int> codepoints;
    for (int i = 0; i < TextLength(characters); i++) {
        codepoints.push_back(static_cast<int>(characters[i]));
    }
    Font font = LoadFontEx("assets/fonts/DejaVuSansMono.ttf", 64, &codepoints[0], 95);

    while (!WindowShouldClose()) {

        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();

        ClearBackground(BLACK);


        // 3D render --------------
        BeginMode3D(camera);

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
    UnloadModel(icosahedron_model);
    UnloadModel(rectangles_model);
    CloseWindow();

    return 0;
}
