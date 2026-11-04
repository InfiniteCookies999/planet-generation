#include "controlled_camera.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <raymath.h>

ControlledCamera::ControlledCamera(Vector3 target, const CameraParams& params)
    : m_camera({
        .position = {0, 0, 0},
        .target = target,
        .up = { 0, 1, 0 },
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    })
    , m_min_zoom(params.min_zoom)
    , m_max_zoom(params.max_zoom) {

    assert(m_min_zoom >= 0.0f);
    assert(m_max_zoom >= 0.0f);
    assert(m_max_zoom >= m_min_zoom);

    m_yaw = params.yaw;
    m_pitch = params.pitch;

    m_distance_to_target = params.zoom;
    m_distance_to_target = std::clamp(m_distance_to_target, m_min_zoom, m_max_zoom);
    // Recompute position in case distance to target was outside the zoom bounds.
    compute_position_from_yaw_and_pitch();
}

void ControlledCamera::update(float dt) {

    if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
        m_start_pan_mouse_position = GetMousePosition();
    }

    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 mouse_d = GetMousePosition() - m_start_pan_mouse_position;

        const float PAN_FACTOR = 0.0005f;
        m_yaw += -mouse_d.x * PAN_FACTOR;
        m_pitch += mouse_d.y * PAN_FACTOR;
    }

    float scroll = GetMouseWheelMove();
    if (scroll != 0.0f) {

        float scroll_modifier = m_distance_to_target / 5.0f;
        float scroll_factor = 0.3f * scroll_modifier;
        m_distance_to_target += -scroll * scroll_factor;
        m_distance_to_target = std::clamp(m_distance_to_target, m_min_zoom, m_max_zoom);
    }

    lerp_to_target(dt);

    compute_position_from_yaw_and_pitch();
    UpdateCamera(&m_camera, CAMERA_CUSTOM);
}

void ControlledCamera::set_target(Vector3 target, bool is_new_target) {
    m_destination_target = target;
    if (is_new_target) {
        caught_up_to_target = false;
        m_destination_target_last_frame = target;
    }
}

Vector3 ControlledCamera::get_forward() const {
    return Vector3Normalize(m_camera.target - m_camera.position);
}

void ControlledCamera::lerp_to_target(float dt) {
    if (caught_up_to_target) {
        m_camera.target = m_destination_target;
        return;
    }

    // Determine how fast the body being targeted is to be used as a stopping
    // condition.
    float target_speed = Vector3Length(m_destination_target - m_destination_target_last_frame);

    float old_distance = Vector3Length(m_destination_target - m_camera.target);

    const float MIN_SPEED = 0.001f;
    const float MAX_SPEED = 6.0f;

    Vector3 step = (m_destination_target - m_camera.target) * 0.5f;
    float speed = Vector3Length(step);
    if (speed < MIN_SPEED) {
        step = Vector3Normalize(m_destination_target - m_camera.target) * MIN_SPEED;
    } else if (speed > MAX_SPEED) {
        step = Vector3Normalize(m_destination_target - m_camera.target) * MAX_SPEED;
    }

    m_camera.target = m_camera.target + step * dt * 24.0f;

    // If the amount the camera moved towards the target is less than its speed then
    // we are considered having caught up to the target.
    //
    // Overstepping should not be possible since this always takes steps to be halfway
    // between the current target and the destination target.
    float new_distance = Vector3Length(m_destination_target - m_camera.target);
    float distance_closed = old_distance - new_distance;
    if (distance_closed < target_speed) {
        caught_up_to_target = true;
        m_camera.target = m_destination_target;
    }

    m_destination_target_last_frame = m_destination_target;
}

void ControlledCamera::compute_position_from_yaw_and_pitch() {
    // Make sure yaw/pitch don't become too large and cause any inaccuracies.
    const float HPI = PI * 0.5f;
    m_yaw = fmodf(m_yaw, 2*PI);
    if (m_pitch < -(HPI - 0.1f)) {
        m_pitch = -(HPI - 0.1f);
    } else if (m_pitch > (HPI - 0.1f)) {
        m_pitch = HPI - 0.1f;
    }

    // forward vector (0, 0, -1) multiplied by rotation matrix Ry * Rx.
    float ax = -(sin(m_yaw) * cos(m_pitch));
    float ay = sin(m_pitch);
    float az = -(cos(m_yaw) * cos(m_pitch));

    m_camera.position.x = m_camera.target.x + ax * m_distance_to_target;
    m_camera.position.y = m_camera.target.y + ay * m_distance_to_target;
    m_camera.position.z = m_camera.target.z + az * m_distance_to_target;
}
