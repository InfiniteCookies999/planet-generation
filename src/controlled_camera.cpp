#include "controlled_camera.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <raymath.h>

ControlledCamera::ControlledCamera(Vector3 _position, Vector3 _target,
                                   float _min_zoom, float _max_zoom)
    : camera({
        .position = _position,
        .target = _target,
        .up = { 0, 1, 0 },
        .fovy = 45.f,
        .projection = CAMERA_PERSPECTIVE
    })
    , min_zoom(_min_zoom)
    , max_zoom(_max_zoom) {

    assert(min_zoom >= 0.0f);
    assert(max_zoom >= 0.0f);
    assert(max_zoom >= min_zoom);

    Vector3 direction_to_target = Vector3Normalize(_position - _target);
    distance_to_target = Vector3Length(_position - _target);
    yaw = atan2(direction_to_target.x, -direction_to_target.z);
    pitch = asin(direction_to_target.y);

    distance_to_target = std::clamp(distance_to_target, min_zoom, max_zoom);
    // Recompute position in case distance to target was outside the zoom bounds.
    compute_position_from_yaw_and_pitch();
}

void ControlledCamera::update() {
    if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
        start_pan_mouse_position = GetMousePosition();
    }

    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 mouse_d = GetMousePosition() - start_pan_mouse_position;

        const float PAN_FACTOR = 0.0005f;
        yaw += -mouse_d.x * PAN_FACTOR;
        pitch += mouse_d.y * PAN_FACTOR;

        compute_position_from_yaw_and_pitch();
    }

    float scroll = GetMouseWheelMove();
    if (scroll != 0.0f) {
        // TODO (maddie): may feel nicer to have the scrolling speed up/slow down depending on
        // how close the camera is to the target. Ex. if the camera is closer to the target the
        // scoll speed slows down and vice versa.
        const float SCROLL_FACTOR = 0.3f;
        distance_to_target += -scroll * SCROLL_FACTOR;
        distance_to_target = std::clamp(distance_to_target, min_zoom, max_zoom);
        compute_position_from_yaw_and_pitch();
    }

    UpdateCamera(&camera, CAMERA_CUSTOM);
}

void ControlledCamera::compute_position_from_yaw_and_pitch() {
    // Make sure yaw/pitch don't become too large and cause any inaccuracies.
    const float HPI = PI * 0.5f;
    yaw = fmodf(yaw, 2*PI);
    if (pitch < -(HPI - 0.1f)) {
        pitch = -(HPI - 0.1f);
    } else if (pitch > (HPI - 0.1f)) {
        pitch = HPI - 0.1f;
    }

    // forward vector (0, 0, -1) multiplied by rotation matrix Ry * Rx.
    float ax = -(sin(yaw) * cos(pitch));
    float ay = sin(pitch);
    float az = -(cos(yaw) * cos(pitch));

    camera.position.x = ax * distance_to_target;
    camera.position.y = ay * distance_to_target;
    camera.position.z = az * distance_to_target;
}
