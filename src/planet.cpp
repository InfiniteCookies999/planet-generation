#include "planet.h"
#include "ex_math.h"
#include "icosphere.h"
#include "star.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <raylib.h>
#include <raymath.h>
#include <utility>

static OrbitalParameters generate_random_orbital_params(
    const Star* star,
    const Planet* prev_planet,
    std::mt19937& gen
) {

    float semi_major_axis;
    float semi_minor_axis;
    float eccentricity;
    float linear_e;

    // Orbital angles
    // --------------------

    std::normal_distribution<float> inclination_dist(0.0f, 2.0f);
    float inclination = std::clamp(inclination_dist(gen), -15.0f, +15.0f);

    std::normal_distribution<float> long_of_ascending_node_dist(0.0f, 360.0f);
    float long_of_ascending_node = long_of_ascending_node_dist(gen);

    std::normal_distribution<float> argument_of_periapsis_dist(0.0f, 360.0f);
    float arg_of_periapsis = argument_of_periapsis_dist(gen);


    if (!prev_planet) {

        float star_radius_accounted = std::max(0.0f, star->radius - 4.0f);
        star_radius_accounted = pow(star_radius_accounted, 1.6f);
        float min_semi_major_axis = 120.0f + star_radius_accounted;

        std::uniform_real_distribution<float> semi_major_axis_dist(
            min_semi_major_axis, min_semi_major_axis + 30.0f
        );
        semi_major_axis = semi_major_axis_dist(gen);

        float e_shape = 2.1f;
        float e_scale = 0.8f;
        std::gamma_distribution<float> eccentricity_dist(e_shape, e_scale);

        eccentricity = eccentricity_dist(gen) / 10.0f;

    } else {
        // The new orbit is constrained such that given a randomly generated angular orientation
        // the new orbit's semi major axis is at least as large as the circle formed by taking the
        // radius of the previous planet's semi major axis and its linear eccentricity plus some
        // epsilon.
        //
        // Then the maximum eccentricity is computed by ensuring that the new semi minor axis does not
        // overlap the circle and that the periapsis does not as well.
        static const float EPS = 35.0f;

        auto& prev_orbital_params = prev_planet->get_orbital_params();

        std::uniform_real_distribution<float> semi_major_axis_dist(2.0f, 20.0f);

        float min_dist = prev_orbital_params.semi_major_axis
                       + prev_orbital_params.linear_eccentricity
                       + EPS;

        semi_major_axis = min_dist + semi_major_axis_dist(gen);

        float max_eccentricity0 = sqrtf(1.0f - (min_dist*min_dist)/(semi_major_axis*semi_major_axis));
        float max_eccentricity1 = (semi_major_axis - min_dist) / semi_major_axis;
        float max_eccentricity = std::min(max_eccentricity0, max_eccentricity1);

        std::normal_distribution<float> eccentricity_dist(max_eccentricity, max_eccentricity * 0.25f);
        float diff = std::abs(max_eccentricity - eccentricity_dist(gen));
        eccentricity = std::max(max_eccentricity - diff, 0.0f);

    }

    // Compute the semi minor axis and linear eccentricity from semi major axis and eccentricity.
    float e = eccentricity;
    float sqr = sqrtf(1.0f - e * e);
    assert(!FloatEquals(sqr, 0.0f) && "Curve should be elliptical");

    semi_minor_axis = semi_major_axis * sqr;
    linear_e = sqrtf(semi_major_axis*semi_major_axis - semi_minor_axis*semi_minor_axis);


    Quaternion inclination_q = QuaternionFromAxisAngle({ 1, 0, 0 }, radians(inclination));
    Quaternion longitude_q = QuaternionFromAxisAngle({ 0, 1, 0 },
        // When inclination is negative the planet ends up ascending on the other end
        // of the orbit. In order to have +X remain as the reference direction then
        // 180 degrees must be added.
        radians(inclination < 0.0f ? long_of_ascending_node + 180.0f : long_of_ascending_node)
    );
    Quaternion periapsis_q = QuaternionFromAxisAngle({ 0, 1, 0 },
        // Same reason as for long of ascending node.
        radians(inclination < 0.0f ? -arg_of_periapsis + 180.0f : -arg_of_periapsis)
    );

    Quaternion inv_transition = QuaternionMultiply(
        longitude_q,
        QuaternionMultiply(
            inclination_q,
            periapsis_q
        )
    );
    Quaternion transition = QuaternionInvert(inv_transition);

    Vector3 plane_normal = Vector3Normalize(Vector3RotateByQuaternion({ 0, 1, 0 }, inv_transition));
    Vector3 plane_xt = Vector3Normalize(Vector3RotateByQuaternion({ 1, 0, 0 }, inv_transition));
    Vector3 plane_zt = Vector3Normalize(Vector3RotateByQuaternion({ 0, 0, 1 }, inv_transition));

    return OrbitalParameters{
        .semi_major_axis = semi_major_axis,
        .semi_minor_axis = semi_minor_axis,
        .eccentricity = eccentricity,
        .linear_eccentricity = linear_e,
        .inclination = inclination,
        .long_of_ascending_node = long_of_ascending_node,
        .arg_of_periapsis = arg_of_periapsis,
        .plane_normal = plane_normal,
        .plane_xt = plane_xt,
        .plane_zt = plane_zt,
        .transition = transition,
        .inv_transition = inv_transition,
        .mean_anomaly = 0,
        .true_anomaly = 0,
    };
}

Planet::Planet(
    Shader planet_shader,
    const Star* star,
    const OrbitalParameters& orbital_params,
    float max_lod_radius
)
    : m_star(star)
    , m_orbital_params(orbital_params)
    , m_max_lod_radius(max_lod_radius)
{
    m_model = LoadModelFromMesh(create_icosphere_mesh(4, m_max_lod_radius));
    m_model.materials[0].shader = planet_shader;
}

Planet Planet::generate_random_planet(
    Shader planet_shader,
    const Star* star,
    const Planet* prev_planet,
    std::mt19937& gen
) {
    OrbitalParameters orbital_params = generate_random_orbital_params(star, prev_planet, gen);

    // 4.0f rough central star size. Use sqrt so the planets do not become
    // very large just because the star does.
    float max_radius = 2.8f/4.0f * std::sqrtf(star->radius);
    float min_radius = 1.6f/4.0f * std::sqrtf(star->radius);
    float mean_radius = (max_radius + min_radius) * 0.5f;

    // Divide by 2 since 2 std deviations results in ~95% of the values between
    // min and max radius being covered.
    float stddev = (mean_radius - min_radius) * 0.5f;
    std::normal_distribution<float> radius_distribution(mean_radius, stddev);

    float radius = radius_distribution(gen);
    radius = std::clamp(radius, min_radius, max_radius);

    return Planet(planet_shader, star, orbital_params, radius);
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

    m_orbital_params.mean_anomaly += 0.001f;
    const float ecc = m_orbital_params.eccentricity;
    const float ecc_anomaly = get_eccentric_anomaly(m_orbital_params.mean_anomaly, ecc);

    // From: https://en.wikipedia.org/wiki/True_anomaly#From_the_eccentric_anomaly
    // Specifically "a form of this equation was derived by R. Broucke and P. Cefola..."
    const float beta = ecc / (1.0f + sqrt(1.0f - ecc*ecc));
    const float true_anomaly = ecc_anomaly + 2.0f * atan((beta * sin(ecc_anomaly)) / (1.0f - beta * cos(ecc_anomaly)));

    m_orbital_params.true_anomaly = true_anomaly;
}

Vector3 Planet::get_position() const {
    // Coordinates within the plane of the orbit.
    // Negate true anomaly to get it to orbit counter-clockwise
    float x = m_orbital_params.semi_major_axis * cos(-m_orbital_params.true_anomaly);
    float z = m_orbital_params.semi_minor_axis * sin(-m_orbital_params.true_anomaly);

    // Transform by the orbit offset relative to the star.
    Matrix rotation = QuaternionToMatrix(m_orbital_params.inv_transition);
    Matrix translation = MatrixTranslate(-m_orbital_params.linear_eccentricity, 0, 0);
    Matrix transform = MatrixMultiply(translation, rotation);
    return m_star->position + Vector3Transform({x, 0, z}, transform);
}

Vector3 Planet::get_orbit_center() const {
    Vector3 offset = Vector3{ -m_orbital_params.linear_eccentricity, 0, 0 };
    return m_star->position + Vector3RotateByQuaternion(offset, m_orbital_params.inv_transition);
}

Vector3 Planet::get_orbit_focus() const {
    return m_star->position;
}

Vector3 Planet::get_orbit_periapsis() const {
    Vector3 offset = Vector3{
        -m_orbital_params.linear_eccentricity + m_orbital_params.semi_major_axis, 0, 0
    };
    return m_star->position + Vector3RotateByQuaternion(offset, m_orbital_params.inv_transition);
}

Vector3 Planet::get_ascending_node() const {

    // Given the parametric equation of an ellipse:
    // E(t) = center + a*cos(t) + b*sin(t)
    //
    // We want to solve t for: E(t) = (x, 0, z)
    // That gives us:
    // 0 = center.y + a.y*cos(t) + b.y*sin(t)
    //
    // For the sake of simplification:
    // center.y = c, a.y = a, b.y = b.
    //
    // To solve we want to transform the equation:
    // a*cos(t) + b*sin(t)  ->  r*cos(t - q)
    //
    // Difference formula for cos:
    // r*cos(t - q) = r*cos(q)*cos(t) + r*sin(q)*sin(t)
    //
    // a = r*cos(q), b = r*sin(q)
    //
    // Solving for r:
    // a^2 + b^2 = r^2 * cos^2(q) + r^2 * sin^2(q)
    // a^2 + b^2 = r^2
    // r = sqrt(a^2 + b^2)
    //
    // Solving for q:
    // q = acos(a/r)
    //
    // From here it is easy enough to plug back into r*cos(t - q) = c
    // and solve for t. That is what the code below does.

    Vector3 center = get_orbit_center();

    Vector3 a = m_orbital_params.plane_xt * m_orbital_params.semi_major_axis;
    Vector3 b = m_orbital_params.plane_zt * m_orbital_params.semi_minor_axis;

    float r = sqrtf(a.y*a.y + b.y*b.y);
    if (FloatEquals(r, 0.0f)) {

        // In this case we want to render the ascending node where the ellipse crosses the
        // reference axis (-x).

        float t = PI; //-m_orbital_params.long_of_ascending_node;
        Vector3 ascending_node = center + a * cos(t) + b * sin(t);

        return ascending_node;
    }

    float cy = center.y - m_star->position.y;

    float sign = b.y >= 0.0f ? +1 : -1;

    float shift = sign * acos(a.y / r);
    float t = acos(-cy / r) + shift;
    // t = shift - (t - shift);   This calculates the descending node.

    Vector3 star_relative_position = Vector3{ center.x, cy + m_star->position.y, center.z };
    Vector3 ascending_node = star_relative_position + a * cos(t) + b * sin(t);
//    std::cout << "ascending_node: " << ascending_node.x << ", " << ascending_node.y << ", " << ascending_node.z << "\n";
    return ascending_node;
}

void Planet::move_from(Planet&& other) noexcept {
    m_orbital_params = other.m_orbital_params;
    m_max_lod_radius = other.m_max_lod_radius;
    m_model = std::exchange(other.m_model, Model{});
    m_star = other.m_star;
}


float Planet::get_eccentric_anomaly(float mean_anomaly, float eccentricity) {
    // Solves for eccentric anomaly using Newton's method.
    //
    // Mean anomaly and eccentric anomaly are related by:
    // mean = E - e*sin(E)
    // Where E is the eccentric anomaly being solved for.
    //
    // Then Newton's method is applied to:
    // f(E) = E - e*sin(E) - mean

    float mean = mean_anomaly;
    float e = eccentricity;
    float x0 = mean;

    const float EPS = 1e-6;

    float xn = x0;
    for (int step = 0; step < 100; step++) {
        xn = xn - (xn - e * sin(xn) - mean) / (1.0f - e * cos(xn));
        if (step >= 15) {
            float sol = xn - e * sin(xn);
            if (std::abs(sol - mean_anomaly) <= EPS) {
                break;
            }
        }
    }

    return xn;
}

Planet::~Planet() {
    UnloadModel(m_model);
}