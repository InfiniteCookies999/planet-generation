#pragma once

#include <raylib.h>

class ControlledCamera {
public:

    /// @param target the point to look at.
    ControlledCamera(Vector3 position, Vector3 target,
                     float min_zoom, float max_zoom);

    void update();

    Camera get_raylib_camera() const {
        return m_camera;
    }

    float get_yaw() const {
        return m_yaw;
    }

    float get_pitch() const {
        return m_pitch;
    }

private:
    Camera m_camera;
    const float m_min_zoom;
    const float m_max_zoom;

    float m_yaw;
    float m_pitch;
    float m_distance_to_target;

    Vector2 m_start_pan_mouse_position = { 0 };

    void compute_position_from_yaw_and_pitch();

};
