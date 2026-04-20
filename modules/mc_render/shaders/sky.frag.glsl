#version 450

layout(push_constant) uniform SkyPush {
    float time_of_day;   /* 0..24000 ticks */
    float screen_width;
    float screen_height;
    float _pad;
} sky;

layout(location = 0) in vec2 frag_uv;
layout(location = 0) out vec4 out_color;

/* ---- Color helpers ---- */

vec3 rgb(float r, float g, float b) {
    return vec3(r / 255.0, g / 255.0, b / 255.0);
}

vec3 lerp_color(vec3 a, vec3 b, float t) {
    return mix(a, b, clamp(t, 0.0, 1.0));
}

/* ---- Hash for star placement ---- */

float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    /* Vertical coordinate: 0 = bottom, 1 = top */
    float y = 1.0 - frag_uv.y;

    float t = sky.time_of_day;

    /* ---- Sky color zones ---- */
    /* Day:   0-11000  (dawn 0-1000) */
    /* Dusk:  11000-13000 */
    /* Night: 13000-23000 */
    /* Dawn:  23000-24000 (wraps to 0-1000) */

    /* Zenith (top) and horizon (bottom) colors */
    vec3 day_zenith  = rgb(135.0, 206.0, 235.0);
    vec3 day_horizon = vec3(1.0);

    vec3 sunset_zenith  = rgb(60.0, 40.0, 80.0);
    vec3 sunset_horizon = rgb(255.0, 100.0, 50.0);

    vec3 night_zenith  = rgb(5.0, 5.0, 20.0);
    vec3 night_horizon = rgb(10.0, 10.0, 30.0);

    vec3 dawn_zenith  = rgb(80.0, 100.0, 140.0);
    vec3 dawn_horizon = rgb(255.0, 160.0, 80.0);

    vec3 zenith;
    vec3 horizon;

    if (t < 1000.0) {
        /* Dawn: 0-1000 */
        float f = t / 1000.0;
        zenith  = lerp_color(dawn_zenith, day_zenith, f);
        horizon = lerp_color(dawn_horizon, day_horizon, f);
    } else if (t < 11000.0) {
        /* Full day */
        zenith  = day_zenith;
        horizon = day_horizon;
    } else if (t < 12000.0) {
        /* Dusk start: day -> sunset */
        float f = (t - 11000.0) / 1000.0;
        zenith  = lerp_color(day_zenith, sunset_zenith, f);
        horizon = lerp_color(day_horizon, sunset_horizon, f);
    } else if (t < 13000.0) {
        /* Dusk end: sunset -> night */
        float f = (t - 12000.0) / 1000.0;
        zenith  = lerp_color(sunset_zenith, night_zenith, f);
        horizon = lerp_color(sunset_horizon, night_horizon, f);
    } else if (t < 23000.0) {
        /* Full night */
        zenith  = night_zenith;
        horizon = night_horizon;
    } else {
        /* Pre-dawn: night -> dawn (23000-24000) */
        float f = (t - 23000.0) / 1000.0;
        zenith  = lerp_color(night_zenith, dawn_zenith, f);
        horizon = lerp_color(night_horizon, dawn_horizon, f);
    }

    /* Gradient: horizon at bottom, zenith at top */
    vec3 sky_color = lerp_color(horizon, zenith, y);

    /* ---- Stars (visible at night) ---- */
    float night_factor = 0.0;
    if (t >= 13000.0 && t < 23000.0) {
        night_factor = 1.0;
    } else if (t >= 12000.0 && t < 13000.0) {
        night_factor = (t - 12000.0) / 1000.0;
    } else if (t >= 23000.0) {
        night_factor = 1.0 - (t - 23000.0) / 1000.0;
    }

    if (night_factor > 0.0 && y > 0.15) {
        /* Grid-based star field */
        vec2 pixel = frag_uv * vec2(sky.screen_width, sky.screen_height);
        vec2 cell = floor(pixel / 4.0);
        float star = hash(cell);
        if (star > 0.997) {
            float twinkle = 0.7 + 0.3 * sin(cell.x * 12.9898 + cell.y * 78.233);
            float brightness = night_factor * twinkle * smoothstep(0.15, 0.4, y);
            sky_color += vec3(brightness);
        }
    }

    out_color = vec4(sky_color, 1.0);
}
