#pragma once

#include <algorithm>
#include <cassert>
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

    /**
     * @param target the point to look at.
     */
    ControlledCamera(Vector3 target, const CameraParams& params);

    void update(float dt);

    /**
     * @param target the point to look at.
     * @param is_new_target even if the point to look at differs the user
     *                      may be looking at a different object than before.
     */
    void set_target(Vector3 target, bool is_new_target);

    Camera get_raylib_camera() const {
        return m_camera;
    }

    Vector3 get_position() const {
        return m_camera.position;
    }

    void set_min_zoom(float min_zoom) {
        m_min_zoom = min_zoom;
        m_distance_to_target = std::max(m_distance_to_target, m_min_zoom);
    }

    void set_max_zoom(float max_zoom) {
        m_max_zoom = max_zoom;
        m_distance_to_target = std::min(m_distance_to_target, m_min_zoom);
    }

    Vector3 get_forward() const;

    Vector3 get_up() const {
        return m_camera.up;
    }

    float get_yaw() const {
        return m_yaw;
    }

    float get_pitch() const {
        return m_pitch;
    }

private:
    Camera m_camera;
    float m_min_zoom;
    float m_max_zoom;

    float m_yaw;
    float m_pitch;
    float m_distance_to_target;

    bool caught_up_to_target = true;
    Vector3 m_destination_target{};
    Vector3 m_destination_target_last_frame{};

    Vector2 m_start_pan_mouse_position{};

    void lerp_to_target(float dt);

    void compute_position_from_yaw_and_pitch();

};
