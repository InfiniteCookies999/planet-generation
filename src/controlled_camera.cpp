#include "controlled_camera.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <raylib.h>
#include <raymath.h>

ControlledCamera::ControlledCamera(Vector3 position, Vector3 target,
                                   float min_zoom, float max_zoom)
    : m_camera({
        .position = position,
        .target = target,
        .up = { 0, 1, 0 },
        .fovy = 45.f,
        .projection = CAMERA_PERSPECTIVE
    })
    , m_min_zoom(min_zoom)
    , m_max_zoom(max_zoom) {

    assert(m_min_zoom >= 0.0f);
    assert(m_max_zoom >= 0.0f);
    assert(m_max_zoom >= m_min_zoom);

    Vector3 direction_to_target = Vector3Normalize(position - target);
    m_distance_to_target = Vector3Length(position - target);
    m_yaw = atan2(direction_to_target.x, -direction_to_target.z);
    m_pitch = asin(direction_to_target.y);

    m_distance_to_target = std::clamp(m_distance_to_target, m_min_zoom, m_max_zoom);
    // Recompute position in case distance to target was outside the zoom bounds.
    compute_position_from_yaw_and_pitch();
}

void ControlledCamera::update() {
    if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
        m_start_pan_mouse_position = GetMousePosition();
    }

    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 mouse_d = GetMousePosition() - m_start_pan_mouse_position;

        const float PAN_FACTOR = 0.0005f;
        m_yaw += -mouse_d.x * PAN_FACTOR;
        m_pitch += mouse_d.y * PAN_FACTOR;

        compute_position_from_yaw_and_pitch();
    }

    float scroll = GetMouseWheelMove();
    if (scroll != 0.0f) {
        // TODO (maddie): may feel nicer to have the scrolling speed up/slow down depending on
        // how close the camera is to the target. Ex. if the camera is closer to the target the
        // scoll speed slows down and vice versa.
        const float SCROLL_FACTOR = 0.3f;
        m_distance_to_target += -scroll * SCROLL_FACTOR;
        m_distance_to_target = std::clamp(m_distance_to_target, m_min_zoom, m_max_zoom);
        compute_position_from_yaw_and_pitch();
    }

    UpdateCamera(&m_camera, CAMERA_CUSTOM);
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

    m_camera.position.x = ax * m_distance_to_target;
    m_camera.position.y = ay * m_distance_to_target;
    m_camera.position.z = az * m_distance_to_target;
}
