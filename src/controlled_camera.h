#pragma once

#include <raylib.h>

class ControlledCamera {
public:

    ControlledCamera(Vector3 _position, Vector3 _target,
                     float _min_zoom, float _max_zoom);

    void update();

    Camera get_raylib_camera() const {
        return camera;
    }

private:
    Camera camera;
    const float min_zoom;
    const float max_zoom;

    float yaw;
    float pitch;
    float distance_to_target;

    Vector2 start_pan_mouse_position = { 0 };

    void compute_position_from_yaw_and_pitch();

};
