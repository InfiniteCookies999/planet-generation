#include "planet.h"
#include "ex_math.h"
#include "icosphere.h"
#include "star.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>
#include <raylib.h>
#include <raymath.h>
#include <utility>

static OrbitalParameters generate_random_orbital_params(
    const Star& star,
    const Planet* prev_planet,
    std::mt19937& gen
) {

    constexpr float ABS_MAX_ECCENTRICITY = 0.85f;
    constexpr float MAJOR_AXIS_SEPARATION = 20.0f;
    constexpr float MINOR_AXIS_SEPARATION = 20.0f;

    float star_radius_add = std::max(0.0f, star.radius - 4.0f);
    star_radius_add = pow(star_radius_add, 1.6f);
    float min_semi_major_axis = 120.0f + star_radius_add;
    float min_semi_minor_axis = 0.0f;
    float max_eccentricity = ABS_MAX_ECCENTRICITY;
    float min_eccentricity = 0.0f;

    if (prev_planet) {
        auto& prev_orbital_params = prev_planet->get_orbital_params();

        min_semi_major_axis = prev_orbital_params.semi_major_axis + MAJOR_AXIS_SEPARATION;
        min_semi_minor_axis = prev_orbital_params.semi_minor_axis + MINOR_AXIS_SEPARATION;
    }

    std::uniform_real_distribution<float> semi_major_axis_dist(
        min_semi_major_axis, min_semi_major_axis + 30.0f
    );

    float semi_major_axis = semi_major_axis_dist(gen);
    if (prev_planet) {
        // Determining the constraints on eccentricity such that the orbits do
        // not overlap each other. Accounts for 3 situations:
        //
        // 1. Overlap due to the eccentricity being large enough that it shrinks
        //    the semi minor axis too much.
        // 2. Overlap due to the resulting linear eccentricity moving the planet
        //    far enough that it overlaps on the apoapsis side.
        // 3. Overlap due to the resulting linear eccentricity moving the planet
        //    far enough that it overlaps on the periapsis side.
        {
            float a = semi_major_axis;
            float b = min_semi_minor_axis;
            max_eccentricity = sqrt(1.0f - (b*b)/(a*a));
        }

        // Computing the distances to the previous apoapsis and periapsis.
        auto& prev_orbital_params = prev_planet->get_orbital_params();
        float prev_apoapsis_dist = prev_orbital_params.semi_major_axis
                                 + prev_orbital_params.linear_eccentricity;
        float prev_periapsis_dist = prev_orbital_params.semi_major_axis
                                  - prev_orbital_params.linear_eccentricity;

        // We want a new minimum away from each so the orbits do not overlap.
        float min_apoapsis_dist = prev_apoapsis_dist + MAJOR_AXIS_SEPARATION;
        float min_periapsis_dist = prev_periapsis_dist + MAJOR_AXIS_SEPARATION;

        // Calculate limits by taking the minimum distances and solving for eccentricity.
        min_eccentricity = (min_apoapsis_dist / semi_major_axis) - 1.0f;
        min_eccentricity = std::max(min_eccentricity, 0.0f);

        float max_eccentricity2 = 1.0f - (min_periapsis_dist / semi_major_axis);
        max_eccentricity = std::min(max_eccentricity, max_eccentricity2);

        // Make sure not to go outside the absolute bounds.
        max_eccentricity = std::min(max_eccentricity, 0.85f);
        min_eccentricity = std::max(min_eccentricity, 0.0f);
    }

    float eccentricity = 0.0f;

    // Use of a gamma distribution so since we intend the value to be positive and
    // more roughly biased towards zero.
    // If the previous planet exists then it follows a similar eccentricity to
    // the last.
    if (!prev_planet) {
        float e_shape = 2.1f;
        float e_scale = 1.5f;
        std::gamma_distribution<float> eccentricity_dist(e_shape, e_scale);

        eccentricity = eccentricity_dist(gen) / 10.0f;
    } else {
        auto& prev_orbital_params = prev_planet->get_orbital_params();
        float prev_eccentricity = prev_orbital_params.eccentricity;

        float s0 = std::abs(prev_eccentricity - min_eccentricity);
        float s1 = std::abs(prev_eccentricity - max_eccentricity);
        float stddev = std::max(s0, s1) * 0.5f;
        std::normal_distribution<float> eccentricity_dist(prev_eccentricity, stddev);

        eccentricity = eccentricity_dist(gen);
    }
    eccentricity = std::clamp(eccentricity, min_eccentricity, max_eccentricity);

    // Compute the semi minor axis and linear eccentricity from semi major axis and eccentricity.
    float e = eccentricity;
    float sqr = sqrtf(1.0f - e * e);
    assert(!FloatEquals(sqr, 0.0f) && "Curve should be elliptical");

    float semi_minor_axis = semi_major_axis * sqr;
    float linear_e = sqrtf(semi_major_axis*semi_major_axis - semi_minor_axis*semi_minor_axis);

    std::normal_distribution<float> inclination_dist(0.0f, 1.0f);
    float inclination = std::clamp(inclination_dist(gen), -15.0f, +15.0f);

    float rad_inclination = radians(inclination);
    // Matrix plane_rot_mat = MatrixRotateZ(radians(inclination));

    // Vector3 plane_normal = Vector3Normalize(Vector3Transform({ 0, 1, 0 }, plane_rot_mat));
    // Vector3 plane_xt = Vector3Normalize(Vector3Transform({ 1, 0, 0 }, plane_rot_mat));
    // Vector3 plane_zt = Vector3Normalize(Vector3Transform({ 0, 0, 1 }, plane_rot_mat));

    // // This transition matrix may look wrong at first but raylib uses column major order
    // // so we are actually storing them within the columns.
    // Matrix transition_mat{};
    // transition_mat.m0 = plane_xt.x;
    // transition_mat.m4 = plane_xt.y;
    // transition_mat.m8 = plane_xt.z;

    // transition_mat.m1 = plane_normal.x;
    // transition_mat.m5 = plane_normal.y;
    // transition_mat.m9 = plane_normal.z;

    // transition_mat.m2  = plane_zt.x;
    // transition_mat.m6  = plane_zt.y;
    // transition_mat.m10 = plane_zt.z;

    // transition_mat.m15 = 1;

    // Quaternion transition = QuaternionFromMatrix(transition_mat);
    // Quaternion inv_transition = QuaternionInvert(transition);

    // TODO (maddie): while this simplifies the code when the argument of periapsis is applied
    // we will likely need to combine the quaternions to get the rotation we want.
    Quaternion transition = QuaternionFromAxisAngle({ 0, 0, -1 }, rad_inclination);
    Quaternion inv_transition = QuaternionInvert(transition);

    Vector3 plane_normal = Vector3Normalize(Vector3RotateByQuaternion({ 0, 1, 0 }, inv_transition));

    return OrbitalParameters{
        .semi_major_axis = semi_major_axis,
        .semi_minor_axis = semi_minor_axis,
        .eccentricity = eccentricity,
        .linear_eccentricity = linear_e,
        .inclination = inclination,
        .plane_normal = plane_normal,
        .transition = transition,
        .inv_transition = inv_transition,
        .true_anomaly = 0,
    };
}

Planet::Planet(
    Shader planet_shader,
    const OrbitalParameters& orbital_params,
    float max_lod_radius
)
    : m_orbital_params(orbital_params)
    , m_max_lod_radius(max_lod_radius)
{
    m_model = LoadModelFromMesh(create_icosphere_mesh(4, m_max_lod_radius));
    m_model.materials[0].shader = planet_shader;
}

Planet Planet::generate_random_planet(
    Shader planet_shader,
    const Star& star,
    const Planet* prev_planet,
    std::mt19937& gen
) {
    OrbitalParameters orbital_params = generate_random_orbital_params(star, prev_planet, gen);

    // 4.0f rough central star size. Use sqrt so the planets do not become
    // very large just because the star does.
    float max_radius = 2.8f/4.0f * std::sqrtf(star.radius);
    float min_radius = 1.6f/4.0f * std::sqrtf(star.radius);
    float mean_radius = (max_radius + min_radius) * 0.5f;

    // Divide by 2 since 2 std deviations results in ~95% of the values between
    // min and max radius being covered.
    float stddev = (mean_radius - min_radius) * 0.5f;
    std::normal_distribution<float> radius_distribution(mean_radius, stddev);

    float radius = radius_distribution(gen);
    radius = std::clamp(radius, min_radius, max_radius);

    return Planet(planet_shader, orbital_params, radius);
}

Planet::Planet(Planet&& other) noexcept {
    move_from(std::move(other));
}

Planet& Planet::operator=(Planet&& other) {
    if (this == &other) {
        return *this;
    }

    move_from(std::move(other));
    return *this;
}

void Planet::update(bool orbit_ring_selected) {

    if (orbit_ring_selected) {
        ++m_orbit_ring_selected_ticks;
        if (m_orbit_ring_selected_ticks > MAX_ORBIT_RING_SELECT_TICKS) {
            m_orbit_ring_selected_ticks = MAX_ORBIT_RING_SELECT_TICKS;
        }
    } else {
        --m_orbit_ring_selected_ticks;
        if (m_orbit_ring_selected_ticks < 0) {
            m_orbit_ring_selected_ticks = 0;
        }
    }

    // TODO (maddie):
    // Proper orbit path so it doesnt have weird slowdown at the ends.
    // Since there is no intent to generate a highly accurate simulation
    // something that approximates it would be good.
    //
    // Probably want to follow kepler orbit:
    // https://en.wikipedia.org/wiki/Kepler_orbit
    m_orbital_params.true_anomaly += 0.0005;
}

Vector3 Planet::get_position(Vector3 star_position) const {

    // Coordinates within the plane of the orbit.
    float x = m_orbital_params.semi_major_axis * cos(m_orbital_params.true_anomaly);
    float z = m_orbital_params.semi_minor_axis * sin(m_orbital_params.true_anomaly);
    float y = 0;

    // Convert to world coordinates.
    Vector3 offset = Vector3RotateByQuaternion({ x, y, z }, m_orbital_params.inv_transition);

    // Vector3 offset = Vector3Transform({ x, y, z }, m_orbital_params.inv_transition);
    // Add additional offset in world units since the orbit is moved to have its foci
    // aligned with the star.
    offset.x += m_orbital_params.linear_eccentricity;
    offset.y += m_orbital_params.linear_eccentricity * tan(radians(m_orbital_params.inclination));
    return star_position + offset;
}

Vector3 Planet::get_orbit_focus(Vector3 star_position) const {
    float focus_y = m_orbital_params.linear_eccentricity * tan(radians(m_orbital_params.inclination));
    return star_position + Vector3{ m_orbital_params.linear_eccentricity, focus_y, 0 };
}

void Planet::move_from(Planet&& other) noexcept {
    m_orbital_params = other.m_orbital_params;
    m_max_lod_radius = other.m_max_lod_radius;
    m_model = std::exchange(other.m_model, Model{});
}

Planet::~Planet() {
    UnloadModel(m_model);
}