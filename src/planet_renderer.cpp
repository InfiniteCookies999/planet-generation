#include "planet_renderer.h"

#include <assert.h>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

PlanetRenderer::PlanetRenderer(Shader planet_shader, Shader orbit_shader)
    : m_shader(planet_shader)
    , m_orbit_shader(orbit_shader)
{
    m_direction_to_star_uniform_loc = GetShaderLocation(m_shader, "u_direction_to_star");

    // m_center_uniform_loc = GetShaderLocation(m_orbit_shader, "u_center");
    m_thickness_uniform_loc = GetShaderLocation(m_orbit_shader, "u_thickness");
    m_semi_major_axis_uniform_loc = GetShaderLocation(m_orbit_shader, "u_semi_major_axis");
    m_semi_minor_axis_uniform_loc = GetShaderLocation(m_orbit_shader, "u_semi_minor_axis");
    m_planar_forward_dot_uniform_loc = GetShaderLocation(m_orbit_shader, "u_planar_forward_dot");
    m_znear_uniform_loc = GetShaderLocation(m_orbit_shader, "u_znear");
    m_zfar_uniform_loc = GetShaderLocation(m_orbit_shader, "u_zfar");
    m_orbit_ring_selected_t_uniform_loc = GetShaderLocation(m_orbit_shader, "u_selected_t");
    m_periapsis_uniform_loc = GetShaderLocation(m_orbit_shader, "u_periapsis");
    m_ascending_node_uniform_loc = GetShaderLocation(m_orbit_shader, "u_ascending_node");
    m_plane_color_uniform_loc = GetShaderLocation(m_orbit_shader, "u_plane_color");

    // Pre-scale the model to be way more than the maximum size ever realistically would be.
    // We want to use a model here because we want to take vertexPosition as passed into the
    // shader in the local scaled coordinates so that easily do calculations in the original
    // orientation within the fragment shader where it thinks xz walks along the plane.
    const float MAX_SEMI_MAJOR_AXIS = 5000.0f;
    float plane_scale = 2.0f * MAX_SEMI_MAJOR_AXIS;
    m_orbit_model = LoadModelFromMesh(GenMeshPlane(plane_scale, plane_scale, 1, 1));
    m_orbit_model.materials[0].shader = orbit_shader;
}

PlanetRenderer::~PlanetRenderer() {
    UnloadModel(m_orbit_model);
}

void PlanetRenderer::render(
    const ControlledCamera& camera,
    const Planet& planet,
    Vector3 star_position,
    bool planet_focused,
    Color plane_color
) const {

    render_orbit(camera, planet, star_position, planet_focused, plane_color);

    // TODO (maddie): Have a planet rotate on its own axis.

    // render planet.
    Vector3 planet_position = planet.get_position(star_position);


    Vector3 direction_to_star = Vector3Normalize(star_position - planet_position);
    SetShaderValue(m_shader, m_direction_to_star_uniform_loc, &direction_to_star, SHADER_UNIFORM_VEC3);

    DrawModel(planet.get_model(), planet_position, 1.0f, WHITE);

}

void PlanetRenderer::render_orbit(
    const ControlledCamera& camera,
    const Planet& planet,
    Vector3 star_position,
    bool planet_focused,
    Color plane_color
) const {

    // TODO (maddie): adjust by argument of periapsis

    auto& orbital_params = planet.get_orbital_params();

    float planar_forward_dot = std::abs(Vector3DotProduct(orbital_params.plane_normal, camera.get_forward()));
    // std::cout << "planar_forward_dot: " << planar_forward_dot << "\n";

    float semi_major_axis = orbital_params.semi_major_axis;
    float semi_minor_axis = orbital_params.semi_minor_axis;

    // render orbit path.
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    BeginShaderMode(m_orbit_shader);

    float znear = rlGetCullDistanceNear();
    float zfar = rlGetCullDistanceFar();

    // TODO (maddie): once the camera is zoomed in on the planet the orbit ring should also fade out.

    int selected_ticks = planet.get_orbit_ring_selected_ticks();
    float orbit_ring_selected_t = static_cast<float>(selected_ticks) / Planet::MAX_ORBIT_RING_SELECT_TICKS;
    if (planet_focused) {
        orbit_ring_selected_t = 1.0f;
    }

    Vector3 periapsis = planet.get_orbit_periapsis(star_position);
    Vector3 ascending_node = planet.get_ascending_node(star_position);


    float plane_color_buffer[] = {
        plane_color.r / 255.0f,
        plane_color.g / 255.0f,
        plane_color.b / 255.0f,
    };

    const float thickness = 0.8f;
    // SetShaderValue(m_orbit_shader, m_center_uniform_loc, &focus, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_orbit_shader, m_thickness_uniform_loc, &thickness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_semi_major_axis_uniform_loc, &semi_major_axis, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_semi_minor_axis_uniform_loc, &semi_minor_axis, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_planar_forward_dot_uniform_loc, &planar_forward_dot, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_znear_uniform_loc, &znear, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_zfar_uniform_loc, &zfar, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_orbit_ring_selected_t_uniform_loc, &orbit_ring_selected_t, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_periapsis_uniform_loc, &periapsis, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_orbit_shader, m_ascending_node_uniform_loc, &ascending_node, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_orbit_shader, m_plane_color_uniform_loc, plane_color_buffer, SHADER_UNIFORM_VEC3);

    // TODO (maddie): this is not using the transition quaternion to rotate the plane.

    // std::cout << "focus: " << focus.x << "," << focus.y << "," << focus.z << "\n";

    // rlPushMatrix();
    // Matrix m = QuaternionToMatrix(orbital_params.inv_transition);
    // Matrix m1 = MatrixTranslate(focus.x, focus.y, focus.z);
    // // m1 = MatrixMultiply(m1, m);
    // float16 m_values = MatrixToFloatV(m1);
    // rlMultMatrixf(&m_values.v[0]);
    // // rlTranslatef(focus.x, focus.y, focus.z);


    // DrawPlane({0,0,0}, {300.0f,300.0f}, WHITE);
    // rlPopMatrix();

    // Vector3 rot_axis = { 0, 0, 1 };
    // Vector3 scale = { 1, 1, 1 };
    // DrawModelEx(m_orbit_model, focus, rot_axis, orbital_params.inclination, scale, WHITE);

    // We want to construct a matrix which first translates to the orbit's foci to
    // the where the star would be in the orbital plane. However, this still takes
    // place in the oribtal coordinates and is not offsetting to the star location
    // till after transitioning to world coordinates.
    // The offset to the star must happen after transitioning into world coordinates
    // since the star's position is defined in world coordinates.
    //
    // ** !! NOTE: Raylib does not follow standard mathematical conventions and MatrixMultiply actually
    // does its operations in left to right order instead of right to left.

    // Our orbit model is defined in the orbital plane we must invert it to go from orbital
    // coordinates to world coordinates.
    Matrix rotation = QuaternionToMatrix(orbital_params.inv_transition);
    Matrix translation = MatrixTranslate(-orbital_params.linear_eccentricity, 0, 0);
    Matrix star_translation = MatrixTranslate(star_position.x, star_position.y, star_position.z);
    m_orbit_model.transform = MatrixMultiply(MatrixMultiply(translation, rotation), star_translation);

    m_orbit_model.materials[m_orbit_model.meshMaterial[0]].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    Material material = m_orbit_model.materials[m_orbit_model.meshMaterial[0]];
    DrawMesh(m_orbit_model.meshes[0], material, m_orbit_model.transform);

    EndShaderMode();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();

}
