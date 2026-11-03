#pragma once

#include <raylib.h>

struct CameraParams {
    float zoom;
    float yaw;
    float pitch;
    float min_zoom;
    float max_zoom;
};

class ControlledCamera {
public:

    /// @param target the point to look at.
    ControlledCamera(Vector3 target, const CameraParams& params);

    void update(float dt);

    void set_target(Vector3 target);

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

    Vector3 m_destination_target{};

    Vector2 m_start_pan_mouse_position{};

    void compute_position_from_yaw_and_pitch();

};
