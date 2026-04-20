#include "mc_math.h"
#include <math.h>

quat_t mc_math_quat_from_euler(float yaw, float pitch, float roll) {
    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);

    quat_t q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    return q;
}

vec3_t mc_math_quat_rotate(quat_t q, vec3_t v) {
    /*
     * Rotate vector v by quaternion q using:
     *   v' = v + 2*w*(qxyz x v) + 2*(qxyz x (qxyz x v))
     * which avoids constructing a rotation matrix.
     */
    vec3_t qv = {q.x, q.y, q.z, 0.0f};
    vec3_t t = mc_math_vec3_scale(mc_math_vec3_cross(qv, v), 2.0f);
    vec3_t result = mc_math_vec3_add(v, mc_math_vec3_add(
        mc_math_vec3_scale(t, q.w),
        mc_math_vec3_cross(qv, t)
    ));
    return result;
}
