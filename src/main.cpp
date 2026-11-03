
#include "billboard_render.h"
#include "controlled_camera.h"
#include "planet_renderer.h"
#include "skybox.h"
#include "star.h"
#include "star_renderer.h"
#include "planet.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>
#include <chrono>

static long long current_time_nanoseconds();
static double current_time_milliseconds();

static std::pair<const Planet*, const Star*> mouse_pick_planet_or_star(
    const ControlledCamera& camera,
    const std::vector<Planet>& planets,
    const Star& star
);

static void render_select_ring(
    const ControlledCamera& camera,
    Shader shader,
    Vector3 center,
    float radius
);

static void generate_random_planets(
    std::vector<Planet>& planets,
    std::mt19937& gen,
    Shader planet_shader
);

int main() {

    InitWindow(1024, 700, "planet generation");

    SetTargetFPS(60);

    Shader planet_shader = LoadShader("assets/shaders/planet.vs", "assets/shaders/planet.fs");
    Shader star_shader = LoadShader(nullptr, "assets/shaders/star.fs");
    Shader star_rays_shader = LoadShader("assets/shaders/star_rays.vs", "assets/shaders/star_rays.fs");
    Shader skybox_shader = LoadShader("assets/shaders/skybox.vs", "assets/shaders/skybox.fs");
    Shader orbit_shader = LoadShader("assets/shaders/orbit.vs", "assets/shaders/orbit.fs");
    Shader select_ring_shader = LoadShader("assets/shaders/select_ring.vs", "assets/shaders/select_ring.fs");

    ControlledCamera camera(
        { 0, 0, 0 },
        CameraParams{
            .zoom = 40.0f,
            .yaw = 0.0f,
            .pitch = 0.4f,
            .min_zoom = 5.0f,
            .max_zoom = +600.0f
        }
    );
    //RL_CULL_DISTANCE_FAR
    //RL_CULL_DISTANCE_FAR
    //rlSetClipPlanes(0.1f, 200.0f);

    std::random_device rd;
    std::mt19937 gen(rd());

    StarRenderer star_renderer(star_shader, star_rays_shader);
    PlanetRenderer planet_renderer(planet_shader, orbit_shader);

    Skybox skybox(skybox_shader, gen);

    int u_tick_loc = GetShaderLocation(star_shader, "u_tick");
    if (u_tick_loc == -1) {
        std::cerr << "** Could not find uniform variable u_tick\n";
        return -1;
    }

    const char* characters = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHI\nJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklm\nnopqrstuvwxyz{|}~";
    std::vector<int> codepoints;
    for (unsigned i = 0; i < TextLength(characters); i++) {
        codepoints.push_back(static_cast<int>(characters[i]));
    }
    Font font_x24 = LoadFontEx("assets/fonts/DejaVuSansMono.ttf", 24, &codepoints[0], 95);

    Star star = create_randomized_star(gen);
    star_renderer.prepare_render(star, gen);

    int num_planets = 4;

    std::vector<Planet> planets;
    generate_random_planets(planets, gen, planet_shader);

    const Planet* focused_planet = nullptr;

    double frame_time = 0;
    int tick = 0;
    while (!WindowShouldClose()) {

        long long start_time = current_time_nanoseconds();

        auto [planet_hit, star_hit] = mouse_pick_planet_or_star(camera, planets, star);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (planet_hit) {
                focused_planet = planet_hit;
            } else if (star_hit) {
                focused_planet = nullptr; // Go back to focusing the star.
            }
        }


        if (focused_planet) {
            camera.set_target(focused_planet->get_position(star.position));
        } else {
            camera.set_target(star.position);
        }
        camera.update(GetFrameTime());


        // Update after so mouse picking uses last frame's position information.
        for (Planet& planet : planets) {
            planet.update();
        }


        BeginDrawing();

        ClearBackground(BLACK);

        // 3D render --------------
        BeginMode3D(camera.get_raylib_camera());

        skybox.render();

        if (planet_hit) {
            if (focused_planet != planet_hit) {
                Vector3 center = planet_hit->get_position(star.position);
                render_select_ring(camera, select_ring_shader, center, planet_hit->get_max_lod_radius());
            }
        } else if (star_hit) {
            if (focused_planet) {
                render_select_ring(camera, select_ring_shader, star.position, star.radius);
            }
        }

        if (IsKeyPressed(KEY_F1)) {
            long long st = current_time_milliseconds();
            star = create_randomized_star(gen);
            star_renderer.prepare_render(star, gen);
            double took = current_time_milliseconds() - st;
            std::cout << std::format("{:.2f}", took) << "ms\n";

            focused_planet = nullptr;
            planets.clear();
            generate_random_planets(planets, gen, planet_shader);
        }

        for (const Planet& planet : planets) {
            planet_renderer.render(camera, planet, star.position);
        }

        star_renderer.render(star, camera, tick);

        EndMode3D();
        // ------------------------

        std::string frame_time_str = std::format("Frame {:.2f} ms", frame_time);
        DrawTextEx(font_x24, frame_time_str.c_str(), {0,0}, 24, 1.0f, WHITE);

        EndDrawing();

        ++tick;

        long long frame_duration = current_time_nanoseconds() - start_time;
        frame_time = frame_duration * 1e-6;
    }

    UnloadFont(font_x24);
    UnloadShader(planet_shader);
    UnloadShader(star_shader);
    UnloadShader(skybox_shader);
    UnloadShader(select_ring_shader);
    CloseWindow();

    return 0;
}

static std::pair<const Planet*, const Star*> mouse_pick_planet_or_star(
    const ControlledCamera& camera,
    const std::vector<Planet>& planets,
    const Star& star
) {
    // TODO (maddie):
    // maybe increase the click distance the further away from the camera it is.

    const Planet* planet_hit = nullptr;

    float closest_distance = std::numeric_limits<float>::max();
    for (const Planet& planet : planets) {
        Vector3 planet_position = planet.get_position(star.position);

        Ray ray = GetScreenToWorldRay(GetMousePosition(), camera.get_raylib_camera());
        float pick_radius = planet.get_max_lod_radius() + 2.0f; // planets can be rather small increase radius.
        RayCollision collision = GetRayCollisionSphere(ray, planet_position, pick_radius);
        if (collision.hit && collision.distance < closest_distance) {
            planet_hit = &planet;
            closest_distance = collision.distance;
        }
    }

    Ray ray = GetScreenToWorldRay(GetMousePosition(), camera.get_raylib_camera());
    RayCollision collision = GetRayCollisionSphere(ray, star.position, star.radius);
    if (collision.hit && collision.distance < closest_distance) {
        return { nullptr, &star };
    }

    return { planet_hit, nullptr };
}

static void render_select_ring(
    const ControlledCamera& camera,
    Shader shader,
    Vector3 center,
    float radius
) {
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    BeginShaderMode(shader);

    int center_uniform_loc = GetShaderLocation(shader, "u_center");
    int radius_uniform_loc = GetShaderLocation(shader, "u_radius");

    SetShaderValue(shader, center_uniform_loc, &center, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, radius_uniform_loc, &radius, SHADER_UNIFORM_FLOAT);

    render_billboard(camera, center, radius + 4.0f);

    EndShaderMode();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

static void generate_random_planets(
    std::vector<Planet>& planets,
    std::mt19937& gen,
    Shader planet_shader
) {

    std::gamma_distribution<float> num_planets_dist(2.5f, 2.0f);
    int num_planets = static_cast<int>(num_planets_dist(gen));
    num_planets = std::clamp(num_planets, 3, 7);

    // std::uniform_int_distribution<int> num_planets_dist(2, 8);
    // int num_planets = num_planets_dist(gen);
    for (int i = 0; i < num_planets; i++) {
        Planet* prev_planet = i != 0
            ? &planets.back()
            : nullptr;
        planets.push_back(Planet::generate_random_planet(planet_shader, prev_planet, gen));
    }
    // Ditribute the planets true anomalies.
    for (int i = 0; i < num_planets; i++) {
        float angle = (i / static_cast<float>(num_planets)) * 2.0f * PI;
        planets[i].set_true_anomaly(angle);
    }

}

static long long current_time_nanoseconds() {
    using namespace std::chrono;
    auto timepoint = steady_clock::now();
    return duration_cast<nanoseconds>(timepoint.time_since_epoch()).count();
}

static double current_time_milliseconds() {
    return current_time_nanoseconds() * 1e-6;
}
