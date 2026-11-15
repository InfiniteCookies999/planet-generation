#pragma once

#include "star.h"

#include <random>
#include <raylib.h>

struct OrbitalParameters {
    float semi_major_axis;
    float semi_minor_axis;

    float eccentricity;
    float linear_eccentricity;

    // As expressed in degrees.
    float inclination;
    float long_of_ascending_node;
    float arg_of_periapsis;

    // The normal of the orbital plane.
    Vector3 plane_normal;
    Vector3 plane_xt;
    Vector3 plane_zt;

    // Rotation from world coordinates to orbital plane coordinates
    // represented by a quaternion.
    Quaternion transition;

    // Inverse rotation from orbital plane coordinates to world coordinates
    // represented by a quaternion.
    Quaternion inv_transition;

    float true_anomaly;

    Vector3 extent_point;
    Vector3 projected_extent_point;
    Vector3 projected_plane_xt;
    Vector3 projected_plane_zt;
    Vector3 outer_extent_point;
};

class Planet {
public:

    Planet(
        Shader planet_shader,
        const OrbitalParameters& orbital_params,
        float max_lod_radius
    );

    /// @param prev_planet The last planet generated that is closer to the
    ///                    star the planets orbit than this next one.
    ///
    static Planet generate_random_planet(
        Shader planet_shader,
        const Star& star,
        const Planet* prev_planet,
        std::mt19937& gen
    );

    Planet(Planet&& other) noexcept;
    Planet& operator=(Planet&&);

    // Don't copy.
    // ---------------------
    Planet(const Planet&) = delete;
    Planet& operator=(const Planet& other) = delete;

    void update(bool orbit_ring_selected);

    Vector3 get_position(Vector3 star_position) const;

    Vector3 get_orbit_center(Vector3 star_position) const;

    Vector3 get_orbit_focus(Vector3 star_position) const;

    Vector3 get_orbit_periapsis(Vector3 star_position) const;

    Vector3 get_ascending_node(Vector3 star_position) const;

    void set_true_anomaly(float true_anomaly) {
        m_orbital_params.true_anomaly = true_anomaly;
    }

    const Model& get_model() const {
        return m_model;
    }

    const OrbitalParameters& get_orbital_params() const {
        return m_orbital_params;
    }

    float get_max_lod_radius() const {
        return m_max_lod_radius;
    }

    int get_orbit_ring_selected_ticks() const {
        return m_orbit_ring_selected_ticks;
    }

    ~Planet();

private:
    OrbitalParameters m_orbital_params;
    float m_max_lod_radius;

    Model m_model;

public:
    static const int MAX_ORBIT_RING_SELECT_TICKS = 20;
private:
    // For how many ticks it has been selected. The value
    // MAX_ORBIT_RING_SELECT_TICKS behaves as an upper limit
    // and once that limit is reached its considered fully
    // selected.
    int m_orbit_ring_selected_ticks = 0;

    void move_from(Planet&& other) noexcept;
};
