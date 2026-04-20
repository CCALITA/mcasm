#include "mc_math.h"

uint8_t mc_math_aabb_intersects(aabb_t a, aabb_t b) {
    return a.min.x <= b.max.x && a.max.x >= b.min.x
        && a.min.y <= b.max.y && a.max.y >= b.min.y
        && a.min.z <= b.max.z && a.max.z >= b.min.z;
}

uint8_t mc_math_aabb_contains(aabb_t box, vec3_t point) {
    return point.x >= box.min.x && point.x <= box.max.x
        && point.y >= box.min.y && point.y <= box.max.y
        && point.z >= box.min.z && point.z <= box.max.z;
}
