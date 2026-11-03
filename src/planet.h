#pragma once

#include <random>
#include <raylib.h>

struct OrbitalParameters {
    float semi_major_axis;
    float semi_minor_axis;

    float eccentricity;
    float linear_eccentricity;

    float true_anomaly;
};

class Planet {
public:

    Planet(Shader planet_shader, const OrbitalParameters& orbital_params);

    /**
     * @param prev_planet The last planet generated that is closer to the
     *                    star the planets orbit than this next one.
     */
    static Planet generate_random_planet(
        Shader planet_shader,
        const Planet* prev_planet,
        std::mt19937& gen
    );

    Planet(Planet&& other) noexcept;
    Planet& operator=(Planet&&);

    // Don't copy.
    // ---------------------
    Planet(const Planet&) = delete;
    Planet& operator=(const Planet& other) = delete;

    void update();

    Vector3 get_position(Vector3 star_position) const;

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

    ~Planet();

private:
    OrbitalParameters m_orbital_params;
    float m_max_lod_radius;

    Model m_model;

    void move_from(Planet&& other) noexcept;
};
