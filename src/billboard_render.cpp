#include "billboard_render.h"
#include "controlled_camera.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

void render_billboard(const ControlledCamera& camera, Vector3 center, float half_size) {

    Camera rl_camera = camera.get_raylib_camera();
    Vector3 forward = Vector3Normalize(rl_camera.target - rl_camera.position);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, rl_camera.up));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));

    rlPushMatrix();
    rlTranslatef(center.x, center.y, center.z);

    rlBegin(RL_QUADS);

        rlTexCoord2f(0, 0);
        rlVertex3f(
            (-right.x * half_size) + (-up.x * half_size),
            (-right.y * half_size) + (-up.y * half_size),
            (-right.z * half_size) + (-up.z * half_size)
        );
        rlTexCoord2f(1, 0);
        rlVertex3f(
            (+right.x * half_size) + (-up.x * half_size),
            (+right.y * half_size) + (-up.y * half_size),
            (+right.z * half_size) + (-up.z * half_size)
        );
        rlTexCoord2f(1, 1);
        rlVertex3f(
            (+right.x * half_size) + (+up.x * half_size),
            (+right.y * half_size) + (+up.y * half_size),
            (+right.z * half_size) + (+up.z * half_size)
        );
        rlTexCoord2f(0, 1);
        rlVertex3f(
            (-right.x * half_size) + (+up.x * half_size),
            (-right.y * half_size) + (+up.y * half_size),
            (-right.z * half_size) + (+up.z * half_size)
        );
    rlEnd();

    rlPopMatrix();
}