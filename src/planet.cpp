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

/// Solves the angle in the equation:
/// a*cos(angle) + b*sin(angle) = c.
///
/// @return The two solutions to the equation and a boolean indicating
///         if true there are two solutions, otherwise no solution or
///         infinite solutions.
///
static std::tuple<float, float, bool> solve_scaled_sin_cos_equation(float a, float b, float c) {
    // To solve the equation first the value of:
    // a*cos(angle) + b*sin(angle)
    //
    // Is rewritten in terms of cos:
    // r*cos(angle - phi)
    //
    // 1. Solving for r:
    //   r*cos(angle - phi) = r*cos(phi)*cos(angle) + r*sin(phi)*sin(angle)  Diff. Formula for cos.
    //   Then:
    //   a = r*cos(phi), b = r*sin(phi)
    //
    //   a^2 + b^2 = r^2 * cos^2(phi) + r^2 * sin^2(phi)
    //             = r^2 * (cos^2(phi) + sin^2(phi))
    //             = r^2
    //   r = sqrt(a^2 + b^2)
    //
    // 2. Solving for phi:
    //   b/a = sin(phi)/cos(phi)
    //       = tan(phi)
    //   phi = atan(b/a)
    //

    if (FloatEquals(a, 0.0f)) {
        // c = b*sin(angle)
        //
        if (FloatEquals(b, 0.0f)) { // Either infinite or no solution.
            return { 0, 0, false };
        }
        float cb = c / b;
        if (cb < -1.0f || cb > 1.0f) {
            return { 0, 0, false };
        }

        float angle0 = asin(cb);
        // the center is always at PI/2
        float angle1 = 0.5f * PI - (angle0 - 0.5f * PI);
        return { angle0, angle1, true };
    }
    if (FloatEquals(b, 0.0f)) {
        // c = a*cos(angle)
        //
        float ca = c / a;
        if (ca < -1.0f || ca > 1.0f) {
            return { 0, 0, false };
        }

        float angle0 = acos(ca);
        float angle1 = -angle0;
        return { angle0, angle1, true };
    }

    float r = sqrtf(a*a + b*b);
    if (FloatEquals(r, 0.0f)) {
        return { 0, 0, false };
    }
    float cr = c / r;
    if (cr < -1.0f || cr > 1.0f) {
        return { 0, 0, false };
    }

    // c/r = cos(angle - phi)
    // angle = acos(c/r) + phi
    // maybe check sign of c/r for quandrant???

    float shift = atan(b / a);
    float angle0 = acos(cr) + shift - (a < 0.0f ? PI : 0.0f);
    float angle1 = shift - (angle0 - shift);

    return { angle0, angle1, true };
}


struct OrbitalOrientation {
    Quaternion transition;
    Quaternion inv_transition;
    Vector3 plane_normal;
    Vector3 plane_xt;
    Vector3 plane_zt;
};

OrbitalOrientation get_orbital_orientation(float inclination, float long_of_ascending_node, float arg_of_periapsis) {

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

    return OrbitalOrientation{
        .transition = transition,
        .inv_transition = inv_transition,
        .plane_normal = plane_normal,
        .plane_xt = plane_xt,
        .plane_zt = plane_zt
    };
}

struct ArgumentOfPeriapsisConstraints {
    Vector3 extent_point;
    Vector3 projected_extent_point;
    Vector3 projected_plane_xt;
    Vector3 projected_plane_zt;
    Vector3 outer_extent_point;
    float max_angle;
};

float solve_projected_y(Vector3 q, Vector3 w, float x, float z) {
    float dem = q.x*w.z - w.x*q.z;
    float a = (x*w.z - z*w.x) / dem;
    float b = (z*q.x - x*q.z) / dem;
    return a*q.y + b*w.y;
}

ArgumentOfPeriapsisConstraints constrain_argument_of_periapsis(
    const OrbitalParameters& prev_orbital_params,
    Vector3 prev_local_extent,
    float linear_eccentricity,
    float inclination,
    float long_of_ascending_node,
    float semi_minor_axis,
    float semi_major_axis
) {

    Matrix rotation = QuaternionToMatrix(prev_orbital_params.inv_transition);
    Matrix translation = MatrixTranslate(-prev_orbital_params.linear_eccentricity, 0, 0);
    Matrix transform = MatrixMultiply(translation, rotation);
    Vector3 extent_point = Vector3Transform(prev_local_extent, transform);

    auto orbital_orientation =
        get_orbital_orientation(inclination, long_of_ascending_node, prev_orbital_params.arg_of_periapsis);

    float sy = solve_projected_y(
        orbital_orientation.plane_xt,
        orbital_orientation.plane_zt,
        extent_point.x,
        extent_point.z
    );
    Vector3 proj_extent_point = { extent_point.x, sy, extent_point.z };

    // maybe we try projecting a point from the world coordinates of the outer rectangle's extent
    // onto the local coordinate and then try and have it be where we would expect. the thing is
    //

    rotation = QuaternionToMatrix(orbital_orientation.transition);
    translation = MatrixTranslate(linear_eccentricity, 0, 0);
    transform = MatrixMultiply(rotation, translation);
    Vector3 local_proj_point = Vector3Transform(proj_extent_point, transform);
    // Vector3 local_proj_point = Vector3RotateByQuaternion(proj_extent_point, orbital_orientation.transition);

    // Vector3 outer_extent = {
    //     -semi_major_axis, 0, semi_minor_axis
    // };
    // Vector3 outer_extent_point;
    // {
    //     rotation = QuaternionToMatrix(orbital_orientation.inv_transition);
    //     translation = MatrixTranslate(-linear_eccentricity, 0, 0);
    //     transform = MatrixMultiply(translation, rotation);
    //     outer_extent_point = Vector3Transform(outer_extent, transform);
    // }
    // Vector3 local_outer_extent_point;
    // {
    //     rotation = QuaternionToMatrix(orbital_orientation.transition);
    //     translation = MatrixTranslate(linear_eccentricity, 0, 0);
    //     transform = MatrixMultiply(rotation, translation);
    //     local_outer_extent_point = Vector3Transform(outer_extent_point, transform);
    // }

    // std::cout << "local_outer_extent_point: " << local_outer_extent_point.x << "," << local_outer_extent_point.z << "\n";


    // std::cout << "local projected point 0: " << local_proj_point.x << "," << local_proj_point.z << "\n";
    // local_proj_point.x += semi_minor_axis;
    // local_proj_point.z -= semi_major_axis;

    //

    std::cout << "semi_minor_axis: " << semi_minor_axis << "\n";
    std::cout << "semi_major_axis: " << semi_major_axis << "\n";
    std::cout << "local projected point 1: " << local_proj_point.x << "," << local_proj_point.z << "\n";

    // This seems almost correct but stopping at corner diagonal to it?
    // auto [max_angle0, max_angle1, has_solution] =
    //     solve_scaled_sin_cos_equation(local_proj_point.z, local_proj_point.x, semi_minor_axis);

    auto [max_angle0, max_angle1, has_solution] =
        solve_scaled_sin_cos_equation(local_proj_point.z, local_proj_point.x, semi_minor_axis);
    max_angle0 = degrees(max_angle0);
    max_angle1 = degrees(max_angle1);
    std::cout << "has solution: " << has_solution << "\n";
    std::cout << "max angle 0: " << max_angle0 << "\n";
    std::cout << "max angle 1: " << max_angle1 << "\n";
    float max_angle = -max_angle0;

    // 0 = x*cos(angle) - z*sin(angle)

    return ArgumentOfPeriapsisConstraints{
        .extent_point = extent_point,
        .projected_extent_point = proj_extent_point,
        .projected_plane_xt = orbital_orientation.plane_xt,
        .projected_plane_zt = orbital_orientation.plane_zt,
        .outer_extent_point = {0,0,0},
        .max_angle = max_angle
    };
}

static OrbitalParameters generate_random_orbital_params(
    const Star& star,
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

        float star_radius_accounted = std::max(0.0f, star.radius - 4.0f);
        star_radius_accounted = pow(star_radius_accounted, 1.6f);
        float min_semi_major_axis = 120.0f + star_radius_accounted;

        std::uniform_real_distribution<float> semi_major_axis_dist(
            min_semi_major_axis, min_semi_major_axis + 30.0f
        );
        semi_major_axis = semi_major_axis_dist(gen);

        // float e_shape = 2.1f;
        // float e_scale = 1.5f;
        // std::gamma_distribution<float> eccentricity_dist(e_shape, e_scale);

        float e_shape = 2.1f;
        float e_scale = 0.8f;
        std::gamma_distribution<float> eccentricity_dist(e_shape, e_scale);

        eccentricity = eccentricity_dist(gen) / 10.0f;

    } else {
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

        eccentricity = max_eccentricity;

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
        .true_anomaly = 0,
        .extent_point = {},
        .projected_extent_point = {},
        .projected_plane_xt = {},
        .projected_plane_zt = {},
        .outer_extent_point = {}
    };



    /*constexpr float ABS_MAX_ECCENTRICITY = 0.85f;
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

    // Orbital angles
    // -----------------------------

    std::normal_distribution<float> inclination_dist(0.0f, 10.0f);
    float inclination = std::clamp(inclination_dist(gen), -15.0f, +15.0f);
    // if (prev_planet) {
    //     inclination = 15.0f;
    // }

    float long_of_ascending_node;
    float arg_of_periapsis = 0;
    if (!prev_planet) {
        std::uniform_real_distribution<float> long_of_ascending_node_dist(0.0f, 360.0f);
        long_of_ascending_node = 0.0f;//long_of_ascending_node_dist(gen);
    } else {
        auto& prev_orbital_params = prev_planet->get_orbital_params();

        float remaining = semi_minor_axis - prev_orbital_params.semi_minor_axis;
        if (remaining > 0.0f) {
            // TODO (maddie): if they are already really close the rotation might miss?

            // TODO (maddie): We do not adjust the px by the inclination. This means if the
            //                current planet has a much higher inclination than the previous
            //                they may overlap. But at that point where they would overlap
            //                may have the orbit above the other so it might not matter. But
            //                shrinking px by inclination may be worth doing.

            // Solves for a maximum angle by finding the first point of contact between
            // the two rectangles formed from the extents of the previous and current
            // orbit prior to rotating.
            //
            // This works by considering the point of intersection with the top left
            // point on the inner rectangle with the rotated outer rectangle as the point
            // of contact. The angle needed to unrotate it such that it aligns with the
            // unrotated outer rectangle is also the angle that would be the maximum.

            float px = -prev_orbital_params.semi_minor_axis;

            float diff_e = linear_e - prev_orbital_params.linear_eccentricity;
            float py = prev_orbital_params.semi_major_axis - diff_e + linear_e;

            float line_x = -semi_minor_axis;


            auto [max_angle0, _, has_solution] = solve_scaled_sin_cos_equation(px, -py, line_x);
            if (has_solution) {
                float max_angle = degrees(max_angle0);

                std::chi_squared_distribution<float> dist(3);
                float angle = std::max(0.0f, max_angle - dist(gen));
                std::uniform_int_distribution<int> pos_dist(0, 1);
                if (pos_dist(gen)) {
                    angle = -angle;
                }
                // std::cout << "max angle: " << max_angle << "\n";
                // std::cout << "angle: " << angle << "\n";

                long_of_ascending_node = prev_orbital_params.long_of_ascending_node + max_angle;//angle;
            } else {
                long_of_ascending_node = prev_orbital_params.long_of_ascending_node;
            }
        } else {
            long_of_ascending_node = prev_orbital_params.long_of_ascending_node;
        }
    }
    // long_of_ascending_node = 0;


    Vector3 extent_point{};
    Vector3 projected_extent_point{};
    Vector3 projected_plane_xt{};
    Vector3 projected_plane_zt{};
    Vector3 outer_extent_point{};
    if (!prev_planet) {
        std::uniform_real_distribution<float> argument_of_periapsis_dist(0.0f, 360.0f);
        arg_of_periapsis = argument_of_periapsis_dist(gen);
        arg_of_periapsis = 0.0f;
    } else {
        auto& prev_orbital_params = prev_planet->get_orbital_params();
        // Vector3 prev_ext0 = {
        //     prev_orbital_params.semi_minor_axis,
        //     0,
        //     prev_orbital_params.semi_major_axis
        // };
        Vector3 prev_ext0 = {
            -prev_orbital_params.semi_major_axis,
            0,
            prev_orbital_params.semi_minor_axis
        };
        auto constraint = constrain_argument_of_periapsis(
            prev_orbital_params,
            prev_ext0,
            linear_e,
            inclination,
            long_of_ascending_node,
            semi_minor_axis,
            semi_major_axis
        );
        extent_point = constraint.extent_point;
        projected_extent_point = constraint.projected_extent_point;
        projected_plane_xt = constraint.projected_plane_xt;
        projected_plane_zt = constraint.projected_plane_zt;
        outer_extent_point = constraint.outer_extent_point;

        arg_of_periapsis = prev_orbital_params.arg_of_periapsis + constraint.max_angle;



        // Vector3 prev_ext0 = {
        //     prev_orbital_params.semi_minor_axis,
        //     0,
        //     prev_orbital_params.semi_major_axis
        // };


        // Vector3 prev_world_ext0;
        // {
        //     Matrix rotation = QuaternionToMatrix(prev_orbital_params.inv_transition);
        //     Matrix translation = MatrixTranslate(-prev_orbital_params.linear_eccentricity, 0, 0);
        //     Matrix transform = MatrixMultiply(translation, rotation);
        //     prev_world_ext0 = Vector3Transform(prev_ext0, transform);
        // }


        // QuaternionFromAxisAngle(plane)

        // world_ext0.xz   ..  this is what we care about.
    }


    // std::cout << "inclination: " << inclination << "\n";
    // std::cout << "long_of_ascending_node: " << long_of_ascending_node << "\n";
    // std::cout << "arg_of_periapsis: " << arg_of_periapsis << "\n";

    // Constructing the transition quaternion using perifocal to equalorial 3-1-3 rotation formula.
    //
    // Rotation about the periapsis happens first in order to rotate within its own plane. This is
    // the same result as rotating by inclination and longitude first and then rotating about the normal
    // of that plane.
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



    // Quaternion inclination_q = QuaternionFromAxisAngle({ 1, 0, 0 }, radians(inclination));

    // Quaternion long_of_ascending_node_q = QuaternionFromAxisAngle({ 0, 1, 0 },
    //     // When inclination is negative the planet ends up ascending on the other end
    //     // of the orbit.
    //     radians(inclination <= 0.0f ? long_of_ascending_node + 180.0f : long_of_ascending_node)
    // );

    // // The angles define how to transition the orbital plane into the world in order to render therefore
    // // they specify the inverse transition.
    // Quaternion inv_transition = QuaternionMultiply(long_of_ascending_node_q, inclination_q);

    // Vector3 plane_normal0 = Vector3Normalize(Vector3RotateByQuaternion({ 0, 1, 0 }, inv_transition));

    // // The argument of periapsis rotates around the plane created from the longitude of the ascending node
    // // and inclination.
    // Quaternion arg_of_periapsis_q = QuaternionFromAxisAngle(plane_normal0,
    //     // When inclination is negative the planet ends up ascending on the other end
    //     // of the orbit.
    //     radians(inclination <= 0.0f ? arg_of_periapsis : arg_of_periapsis + 180.0f)
    // );


    // inv_transition = QuaternionMultiply(arg_of_periapsis_q, inv_transition);
    // Quaternion transition = QuaternionInvert(inv_transition);

    // // With respect to the orbital plane (0, 1, 0) is UP so to get the normal in world coordinates
    // // we must use the inverse matrix to convert from the orbital's UP to the world's UP.
    // Vector3 plane_normal = Vector3Normalize(Vector3RotateByQuaternion({ 0, 1, 0 }, inv_transition));
    // Vector3 plane_xt = Vector3Normalize(Vector3RotateByQuaternion({ 1, 0, 0 }, inv_transition));
    // Vector3 plane_zt = Vector3Normalize(Vector3RotateByQuaternion({ 0, 0, 1 }, inv_transition));*/


    // return OrbitalParameters{
    //     .semi_major_axis = semi_major_axis,
    //     .semi_minor_axis = semi_minor_axis,
    //     .eccentricity = eccentricity,
    //     .linear_eccentricity = linear_e,
    //     .inclination = inclination,
    //     .long_of_ascending_node = long_of_ascending_node,
    //     .arg_of_periapsis = arg_of_periapsis,
    //     .plane_normal = plane_normal,
    //     .plane_xt = plane_xt,
    //     .plane_zt = plane_zt,
    //     .transition = transition,
    //     .inv_transition = inv_transition,
    //     .true_anomaly = 0,
    //     .extent_point = extent_point,
    //     .projected_extent_point = projected_extent_point,
    //     .projected_plane_xt = projected_plane_xt,
    //     .projected_plane_zt = projected_plane_zt,
    //     .outer_extent_point = outer_extent_point
    // };
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
    m_orbital_params.true_anomaly += 0.008f;//0.0005;
}

Vector3 Planet::get_position(Vector3 star_position) const {

    // Coordinates within the plane of the orbit.
    // We are adding PI here since a true anomaly of 0 degrees has the planet at the
    // periabsis of the orbit.
    float x = m_orbital_params.semi_major_axis * sin(m_orbital_params.true_anomaly + PI);
    float z = m_orbital_params.semi_minor_axis * cos(m_orbital_params.true_anomaly + PI);

    // Transform by the orbit offset relative to the star.
    Matrix rotation = QuaternionToMatrix(m_orbital_params.inv_transition);
    Matrix translation = MatrixTranslate(-m_orbital_params.linear_eccentricity, 0, 0);
    Matrix transform = MatrixMultiply(translation, rotation);
    return star_position + Vector3Transform({x, 0, z}, transform);
}

Vector3 Planet::get_orbit_center(Vector3 star_position) const {
    Vector3 offset = Vector3{ -m_orbital_params.linear_eccentricity, 0, 0 };
    return star_position + Vector3RotateByQuaternion(offset, m_orbital_params.inv_transition);
}

Vector3 Planet::get_orbit_focus(Vector3 star_position) const {
    return star_position;
}

Vector3 Planet::get_orbit_periapsis(Vector3 star_position) const {
    Vector3 offset = Vector3{
        -m_orbital_params.linear_eccentricity + m_orbital_params.semi_major_axis, 0, 0
    };
    return star_position + Vector3RotateByQuaternion(offset, m_orbital_params.inv_transition);
}

Vector3 Planet::get_ascending_node(Vector3 star_position) const {

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

    Vector3 center = get_orbit_center(star_position);

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

    float cy = center.y - star_position.y;

    float sign = b.y >= 0.0f ? +1 : -1;

    float shift = sign * acos(a.y / r);
    float t = acos(-cy / r) + shift;
    // t = shift - (t - shift);   This calculates the descending node.

    Vector3 star_relative_position = Vector3{ center.x, cy + star_position.y, center.z };
    Vector3 ascending_node = star_relative_position + a * cos(t) + b * sin(t);
//    std::cout << "ascending_node: " << ascending_node.x << ", " << ascending_node.y << ", " << ascending_node.z << "\n";
    return ascending_node;
}

void Planet::move_from(Planet&& other) noexcept {
    m_orbital_params = other.m_orbital_params;
    m_max_lod_radius = other.m_max_lod_radius;
    m_model = std::exchange(other.m_model, Model{});
}

Planet::~Planet() {
    UnloadModel(m_model);
}