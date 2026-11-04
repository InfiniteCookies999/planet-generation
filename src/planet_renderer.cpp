#include "planet_renderer.h"

#include <assert.h>
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

PlanetRenderer::PlanetRenderer(Shader planet_shader, Shader orbit_shader)
    : m_shader(planet_shader)
    , m_orbit_shader(orbit_shader)
{
    m_direction_to_star_uniform_loc = GetShaderLocation(m_shader, "u_direction_to_star");

    m_center_uniform_loc = GetShaderLocation(m_orbit_shader, "u_center");
    m_thickness_uniform_loc = GetShaderLocation(m_orbit_shader, "u_thickness");
    m_semi_major_axis_uniform_loc = GetShaderLocation(m_orbit_shader, "u_semi_major_axis");
    m_semi_minor_axis_uniform_loc = GetShaderLocation(m_orbit_shader, "u_semi_minor_axis");
    m_planar_forward_dot_uniform_loc = GetShaderLocation(m_orbit_shader, "u_planar_forward_dot");
    m_znear_uniform_loc = GetShaderLocation(m_orbit_shader, "u_znear");
    m_zfar_uniform_loc = GetShaderLocation(m_orbit_shader, "u_zfar");
    m_orbit_ring_selected_t = GetShaderLocation(m_orbit_shader, "u_selected_t");
}

void PlanetRenderer::render(
    const ControlledCamera& camera,
    const Planet& planet,
    Vector3 star_position,
    bool planet_focused
) const {

    render_orbit(camera, planet, star_position, planet_focused);

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
    bool planet_focused
) const {

    // TODO (maddie): adjust by argument of periapsis

    // TODO (maddie): Apply inclination

    float planar_forward_dot = std::abs(Vector3DotProduct({ 0, 1, 0 }, camera.get_forward()));

    auto& orbital_params = planet.get_orbital_params();
    float semi_major_axis = orbital_params.semi_major_axis;
    float semi_minor_axis = orbital_params.semi_minor_axis;

    Vector3 focus = star_position + Vector3{ orbital_params.linear_eccentricity, 0, 0 };

    // render orbit path.
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    BeginShaderMode(m_orbit_shader);

    float znear = rlGetCullDistanceNear();
    float zfar = rlGetCullDistanceFar();

    // TODO (maddie): when the orbit ring selects/deselects it would probably be good to have
    // it fade in and out because at the moment it just kinda instantly flickers.

    // TODO (maddie): once the camera is zoomed in on the planet the orbit ring should also fade out.

    int selected_ticks = planet.get_orbit_ring_selected_ticks();
    float orbit_ring_selected_t = static_cast<float>(selected_ticks) / Planet::MAX_ORBIT_RING_SELECT_TICKS;
    if (planet_focused) {
        orbit_ring_selected_t = 1.0f;
    }

    const float thickness = 0.8f;
    SetShaderValue(m_orbit_shader, m_center_uniform_loc, &focus, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_orbit_shader, m_thickness_uniform_loc, &thickness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_semi_major_axis_uniform_loc, &semi_major_axis, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_semi_minor_axis_uniform_loc, &semi_minor_axis, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_planar_forward_dot_uniform_loc, &planar_forward_dot, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_znear_uniform_loc, &znear, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_zfar_uniform_loc, &zfar, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_orbit_shader, m_orbit_ring_selected_t, &orbit_ring_selected_t, SHADER_UNIFORM_FLOAT);

    Vector2 orbital_plane_size = {
        2.0f * (semi_major_axis + thickness) + 10.0f,
        2.0f * (semi_minor_axis + thickness) + 10.0f,
    };
    DrawPlane(focus, orbital_plane_size, WHITE);

    EndShaderMode();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();

}
