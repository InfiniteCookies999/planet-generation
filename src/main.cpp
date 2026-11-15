
#include "billboard_render.h"
#include "controlled_camera.h"
#include "ex_math.h"
#include "planet_renderer.h"
#include "skybox.h"
#include "star.h"
#include "star_renderer.h"
#include "planet.h"
#include "text_3d.h"

#include <cmath>
#include <fstream>
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

struct MousePick {
    enum class Type {
        NONE,
        PLANET,
        STAR,
        PLANET_ORBIT_RING
    } type;

    union {
        const Planet* planet;
        const Star* star;
    };
};

static MousePick mouse_pick_planet_or_star(
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
    const Star& star,
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
    Shader discard_alpha_shader = LoadShader(NULL, "assets/shaders/discard_zero_alpha.fs");

    ControlledCamera camera(
        { 0, 0, 0 },
        CameraParams{
            .zoom = 40.0f,
            .yaw = 0.0f,
            .pitch = 0.4f,
            .min_zoom = 5.0f,
            .max_zoom = +2000.0f
        }
    );
    //RL_CULL_DISTANCE_FAR
    //RL_CULL_DISTANCE_FAR
    rlSetClipPlanes(0.01f, 3000.0f);

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
    Font font_x64 = LoadFontEx("assets/fonts/DejaVuSansMono.ttf", 64, &codepoints[0], 95);

    Star star = create_randomized_star(gen);
    star_renderer.prepare_render(star, gen);

    std::vector<Planet> planets;
    generate_random_planets(star, planets, gen, planet_shader);

    const Planet* prev_focused_planet = nullptr;
    const Planet* focused_planet = nullptr;

    double frame_time = 0;
    int tick = 0;
    while (!WindowShouldClose()) {

        long long start_time = current_time_nanoseconds();

        MousePick mouse_pick = mouse_pick_planet_or_star(camera, planets, star);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            mouse_pick.type != MousePick::Type::NONE) {

            if (mouse_pick.type == MousePick::Type::PLANET ||
                mouse_pick.type == MousePick::Type::PLANET_ORBIT_RING) {
                focused_planet = mouse_pick.planet;
            } else if (mouse_pick.type == MousePick::Type::STAR) {
                focused_planet = nullptr; // Go back to focusing the star.
            }
        }
        bool planet_focus_changed = prev_focused_planet != focused_planet;
        prev_focused_planet = focused_planet;


        // Update after so mouse picking uses last frame's position information.
        for (Planet& planet : planets) {
            bool orbit_ring_selected = false;
            if (mouse_pick.type == MousePick::Type::PLANET_ORBIT_RING ||
                mouse_pick.type == MousePick::Type::PLANET) {
                orbit_ring_selected = mouse_pick.planet == &planet;
            }
            planet.update(orbit_ring_selected);
        }

        // Set target after updating planets so it focuses correctly on what
        // is being rendered.
        if (focused_planet) {
            camera.set_target(focused_planet->get_position(star.position), planet_focus_changed);
            camera.set_min_zoom(focused_planet->get_max_lod_radius() + 2.0f);
        } else {
            camera.set_target(star.position, planet_focus_changed);
            camera.set_min_zoom(star.radius + 3.0f);
        }
        camera.update(GetFrameTime());



        BeginDrawing();

        ClearBackground(BLACK);

        // 3D render --------------
        BeginMode3D(camera.get_raylib_camera());

        skybox.render();

        switch (mouse_pick.type) {
        case MousePick::Type::PLANET: {
            if (focused_planet != mouse_pick.planet) {
                const Planet* hit_planet = mouse_pick.planet;
                Vector3 center = hit_planet->get_position(star.position);
                render_select_ring(camera, select_ring_shader, center, hit_planet->get_max_lod_radius());
            }
            break;
        }
        case MousePick::Type::STAR: {
            if (focused_planet) {
                render_select_ring(camera, select_ring_shader, star.position, star.radius);
            }
            break;
        }
        default:
            break;
        }

        if (IsKeyPressed(KEY_F1)) {
            long long st = current_time_milliseconds();
            star = create_randomized_star(gen);
            star_renderer.prepare_render(star, gen);
            double took = current_time_milliseconds() - st;
            std::cout << std::format("{:.2f}", took) << "ms\n";

            focused_planet = nullptr;
            planets.clear();
            generate_random_planets(star, planets, gen, planet_shader);
        }

        const float GRID_SIZE = 50.0f;
        // DrawGrid(GRID_SIZE, 1.0f);
        // { // Render coordinates text.
        //     BeginShaderMode(discard_alpha_shader);

        //     draw_text_3d(font_x64, "+X", Vector3{ +GRID_SIZE/2 - 1, 0, 0 }, 0.5f,  0.5f, 0.0f, true, BLUE);
        //     draw_text_3d(font_x64, "-X", Vector3{ -GRID_SIZE/2, 0, 0 }, 0.5f,  0.5f, 0.0f, true, BLUE);

        //     draw_text_3d(font_x64, "+Z", Vector3{ 0, 0, +GRID_SIZE/2 - 1 }, 0.5f,  0.5f, 0.0f, true, BLUE);
        //     draw_text_3d(font_x64, "-Z", Vector3{ 0, 0, -GRID_SIZE/2 }, 0.5f,  0.5f, 0.0f, true, BLUE);

        //     EndShaderMode();
        // }


        Vector3 focus = planets[0].get_orbit_focus(star.position);

        Vector3 normal = planets[0].get_orbital_params().plane_normal;
        Vector3 plane_xt = planets[0].get_orbital_params().plane_xt;
        Vector3 plane_zt = planets[0].get_orbital_params().plane_zt;


        Vector3 mouse_point{};
        {
            const Planet& planet = planets[0];
            Ray ray = GetScreenToWorldRay(GetMousePosition(), camera.get_raylib_camera());
            Vector3 focus = planet.get_orbit_focus(star.position);

            Vector3 p0 = focus;
            Vector3 l0 = ray.position;
            Vector3 plane_normal = planet.get_orbital_params().plane_normal;
            float dem = Vector3DotProduct(ray.direction, plane_normal);
            if (!FloatEquals(dem, 0.0f)) {
                float d = Vector3DotProduct(p0 - l0, plane_normal) / dem;
                mouse_point = l0 + ray.direction * d;
            }
        }




        int cc = 0;
        for (const Planet& planet : planets) {
            planet_renderer.render(
                camera,
                planet,
                star.position,
                focused_planet == &planet,
                cc == 0 ? WHITE : PURPLE
            );
            ++cc;
        }

        // Vector3 extent_point = planets[1].get_orbital_params().extent_point;
        // DrawCircle3D(extent_point, 3.0f, {0, 1, 0}, 0.0f, GREEN);

        // Vector3 proj_extent_point = planets[1].get_orbital_params().projected_extent_point;
        // DrawCircle3D(proj_extent_point, 3.0f, {0, 1, 0}, 0.0f, RED);

        // Vector3 outer_extent_point = planets[1].get_orbital_params().outer_extent_point;
        // DrawCircle3D(outer_extent_point, 3.0f, {0, 1, 0}, 0.0f, YELLOW);

        // DrawLine3D(normal * -20, normal * 20, RED);
        // DrawLine3D(plane_xt * -20, plane_xt * 20, RED);proj_extent_point
        // DrawLine3D(plane_zt * -20, plane_zt * 20, RED);


        Vector3 center = planets[1].get_orbit_center(star.position);
        Vector3 proj_plane_xt = planets[1].get_orbital_params().projected_plane_xt;
        Vector3 proj_plane_zt = planets[1].get_orbital_params().projected_plane_zt;
        DrawLine3D(center, center + proj_plane_xt * planets[1].get_orbital_params().semi_major_axis, YELLOW);
        DrawLine3D(center, center + proj_plane_zt * planets[1].get_orbital_params().semi_minor_axis, YELLOW);


        // Vector3 start = star.position;
        // float d = planets[1].get_orbital_params().distance_thing;
        // //planets[1].get_orbital_params().semi_major_axis
        // Vector3 end = star.position - (plane_xt * d);
        // DrawLine3D(start, end, YELLOW);


        // Vector3 center = planets[0].get_orbit_center(star.position);
        // DrawCircle3D(center, 3.0f, {0, 1, 0}, 0.0f, GREEN);

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
    UnloadFont(font_x64);
    UnloadShader(planet_shader);
    UnloadShader(star_shader);
    UnloadShader(skybox_shader);
    UnloadShader(select_ring_shader);
    UnloadShader(discard_alpha_shader);
    CloseWindow();

    return 0;
}

static MousePick mouse_pick_planet_or_star(
    const ControlledCamera& camera,
    const std::vector<Planet>& planets,
    const Star& star
) {

    MousePick pick{};
    pick.type = MousePick::Type::NONE;

    Ray ray = GetScreenToWorldRay(GetMousePosition(), camera.get_raylib_camera());

    float closest_distance = std::numeric_limits<float>::max();
    for (const Planet& planet : planets) {
        Vector3 planet_position = planet.get_position(star.position);

        float pick_radius = planet.get_max_lod_radius() + 2.0f; // planets can be rather small increase radius.
        RayCollision collision = GetRayCollisionSphere(ray, planet_position, pick_radius);
        if (collision.hit && collision.distance < closest_distance) {
            pick.type = MousePick::Type::PLANET;
            pick.planet = &planet;
            closest_distance = collision.distance;
            continue; // Don't continue to check for ring intersection.
        }



        // Intersection code with orbit ring.
        auto& orbital_params = planet.get_orbital_params();

        // Use the center point as the point on the plane.
        Vector3 center = planet.get_orbit_center(star.position);

        Vector3 p0 = center;
        Vector3 l0 = ray.position;
        Vector3 plane_normal = orbital_params.plane_normal;
        float dem = Vector3DotProduct(ray.direction, plane_normal);
        if (FloatEquals(dem, 0.0f)) {
            // Forward is parallel to the plane.
            continue;
        }
        float d = Vector3DotProduct(p0 - l0, plane_normal) / dem;
        if (d >= closest_distance) {
            continue;
        }

        Vector3 hit_point = l0 + ray.direction * d;

        float ratio = orbital_params.semi_major_axis / orbital_params.semi_minor_axis;

        // Apply transition to do the math within the coordinates of the orbital plane.
        Vector3 t_center = Vector3RotateByQuaternion(center, orbital_params.transition);
        Vector3 t_hit_point = Vector3RotateByQuaternion(hit_point, orbital_params.transition);


        Vector2 offset = Vector2{ t_hit_point.x, t_hit_point.z } - Vector2{ t_center.x, t_center.z };
        offset = Vector2{ offset.x, offset.y * ratio };
        float dist = Vector2Length(offset);

        float planar_forward_dot = std::abs(
            Vector3DotProduct(orbital_params.plane_normal, camera.get_forward())
        );
        float distance_from_camera = Vector3Distance(hit_point, camera.get_position());
        float zfar = rlGetCullDistanceFar();
        float linear_depth = distance_from_camera / zfar;


        float local_dist = std::abs(dist - orbital_params.semi_major_axis);

        float depth_impact = std::max(0.0f, 1.0f - planar_forward_dot);
        depth_impact = pow(depth_impact, 0.25) * 0.7;

        // Note: basically just number tuning.
        float thickness = 3.0f + 3.0f * powf(2.4f, 1.0f + 2.0f * std::min(1.0f, linear_depth)) * depth_impact;
        if (local_dist < thickness) {
            pick.type = MousePick::Type::PLANET_ORBIT_RING;
            pick.planet = &planet;
            closest_distance = d;
        }
    }

    RayCollision collision = GetRayCollisionSphere(ray, star.position, star.radius);
    if (collision.hit && collision.distance < closest_distance) {
        pick.type = MousePick::Type::STAR;
        pick.star = &star;
    }

    return pick;
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
    const Star& star,
    std::vector<Planet>& planets,
    std::mt19937& gen,
    Shader planet_shader
) {

    std::gamma_distribution<float> num_planets_dist(2.5f, 2.0f);
    int num_planets = static_cast<int>(num_planets_dist(gen));
    num_planets = std::clamp(num_planets, 3, 7);

    for (int i = 0; i < num_planets; i++) {
        Planet* prev_planet = i != 0
            ? &planets.back()
            : nullptr;
        planets.push_back(Planet::generate_random_planet(planet_shader, star, prev_planet, gen));
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
