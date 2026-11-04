#pragma once

#include "controlled_camera.h"
#include "planet.h"

#include <raylib.h>

class PlanetRenderer {
public:

    PlanetRenderer(Shader planet_shader, Shader orbit_shader);

    // Don't move or copy.
    // ---------------------
    PlanetRenderer(const PlanetRenderer&) = delete;
    PlanetRenderer(PlanetRenderer&&) = delete;
    PlanetRenderer& operator=(const PlanetRenderer&) = delete;
    PlanetRenderer& operator=(PlanetRenderer&&) = delete;

    void render(
        const ControlledCamera& camera,
        const Planet& planet,
        Vector3 star_position,
        bool planet_focused
    ) const;

private:
    const Shader m_shader;
    const Shader m_orbit_shader;

    // planet shader uniform locations.
    int m_direction_to_star_uniform_loc;

    // orbital shader uniforms locations.
    int m_center_uniform_loc;
    int m_thickness_uniform_loc;
    int m_semi_major_axis_uniform_loc;
    int m_semi_minor_axis_uniform_loc;
    int m_planar_forward_dot_uniform_loc;
    int m_znear_uniform_loc;
    int m_zfar_uniform_loc;
    int m_orbit_ring_selected_t;

    void render_orbit(
        const ControlledCamera& camera,
        const Planet& planet,
        Vector3 star_position,
        bool planet_focused
    ) const;

};