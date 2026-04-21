#include "render_internal.h"
#include <string.h>
#include <stdio.h>

/* ---- Sky push constant layout (must match sky.frag.glsl) ---- */
typedef struct {
    float time_of_day;
    float screen_width;
    float screen_height;
    float _pad;
} sky_push_t;

/* ---- Pipeline creation ---- */

mc_error_t sky_create_pipeline(void)
{
    /* Push constant range for fragment shader */
    VkPushConstantRange push_range;
    memset(&push_range, 0, sizeof(push_range));
    push_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push_range.offset     = 0;
    push_range.size       = sizeof(sky_push_t);

    VkPipelineLayoutCreateInfo layout_ci;
    memset(&layout_ci, 0, sizeof(layout_ci));
    layout_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_ci.pushConstantRangeCount = 1;
    layout_ci.pPushConstantRanges    = &push_range;

    VkResult res = vkCreatePipelineLayout(g_render.device, &layout_ci, NULL,
                                          &g_render.sky_pipeline_layout);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "sky: vkCreatePipelineLayout failed: %d\n", res);
        return MC_ERR_VULKAN;
    }

    /*
     * Pipeline creation requires compiled SPIR-V shaders (sky.vert.spv / sky.frag.spv).
     * Like the terrain pipeline, actual shader loading is deferred.
     * The pipeline handle stays VK_NULL_HANDLE until shaders are available.
     */
    g_render.sky_pipeline = VK_NULL_HANDLE;
    return MC_OK;
}

void sky_destroy_pipeline(void)
{
    if (!g_render.device) return;

    if (g_render.sky_pipeline) {
        vkDestroyPipeline(g_render.device, g_render.sky_pipeline, NULL);
        g_render.sky_pipeline = VK_NULL_HANDLE;
    }
    if (g_render.sky_pipeline_layout) {
        vkDestroyPipelineLayout(g_render.device, g_render.sky_pipeline_layout, NULL);
        g_render.sky_pipeline_layout = VK_NULL_HANDLE;
    }
}

/* ---- Drawing ---- */

void sky_draw(VkCommandBuffer cmd)
{
    if (!cmd || !g_render.sky_pipeline) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_render.sky_pipeline);

    sky_push_t push;
    push.time_of_day  = g_render.world_time;
    push.screen_width  = (float)g_render.swapchain_extent.width;
    push.screen_height = (float)g_render.swapchain_extent.height;
    push._pad          = 0.0f;

    vkCmdPushConstants(cmd, g_render.sky_pipeline_layout,
                       VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);

    /* Draw fullscreen triangle (3 vertices, no vertex buffer needed) */
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

/* ---- Fog adjustment based on time of day ---- */

static vec3_t lerp_vec3(vec3_t a, vec3_t b, float t)
{
    vec3_t result;
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    result.z = a.z + (b.z - a.z) * t;
    result._pad = 0.0f;
    return result;
}

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void sky_update_fog(void)
{
    float t = g_render.world_time;

    /* Day fog: white, far distance */
    vec3_t day_fog   = {1.0f, 1.0f, 1.0f, 0.0f};
    float  day_start = 120.0f;
    float  day_end   = 160.0f;

    /* Night fog: dark, shorter distance */
    vec3_t night_fog   = {0.04f, 0.04f, 0.08f, 0.0f};
    float  night_start = 60.0f;
    float  night_end   = 100.0f;

    float night_factor = 0.0f;
    if (t >= 13000.0f && t < 23000.0f) {
        night_factor = 1.0f;
    } else if (t >= 12000.0f && t < 13000.0f) {
        night_factor = (t - 12000.0f) / 1000.0f;
    } else if (t >= 23000.0f) {
        night_factor = 1.0f - (t - 23000.0f) / 1000.0f;
    } else if (t < 1000.0f) {
        night_factor = 1.0f - t / 1000.0f;
        night_factor = clampf(night_factor, 0.0f, 0.3f);
    }

    g_render.fog_color = lerp_vec3(day_fog, night_fog, night_factor);
    g_render.fog_start = day_start + (night_start - day_start) * night_factor;
    g_render.fog_end   = day_end + (night_end - day_end) * night_factor;

    /* Update clear color to match horizon */
    float day_r = 135.0f / 255.0f, day_g = 206.0f / 255.0f, day_b = 235.0f / 255.0f;
    float night_r = 10.0f / 255.0f, night_g = 10.0f / 255.0f, night_b = 30.0f / 255.0f;

    g_render.clear_r = day_r + (night_r - day_r) * night_factor;
    g_render.clear_g = day_g + (night_g - day_g) * night_factor;
    g_render.clear_b = day_b + (night_b - day_b) * night_factor;
}
