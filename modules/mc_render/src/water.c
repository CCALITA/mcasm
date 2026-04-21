#include "render_internal.h"
#include <string.h>
#include <stdio.h>

/*
 * Water pipeline: uses the same vertex format as terrain but with
 * alpha blending enabled and depth writes disabled.
 *
 * Push constants: mat4 mvp (64 bytes) + float time (4 bytes) = 68 bytes.
 */

mc_error_t vk_create_water_pipeline(void)
{
    /* Pipeline layout with push constants: mat4 (64 bytes) + float (4 bytes) */
    VkPushConstantRange push_range;
    memset(&push_range, 0, sizeof(push_range));
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset     = 0;
    push_range.size       = sizeof(float) * 16 + sizeof(float); /* mat4 + time */

    VkPipelineLayoutCreateInfo layout_ci;
    memset(&layout_ci, 0, sizeof(layout_ci));
    layout_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_ci.pushConstantRangeCount = 1;
    layout_ci.pPushConstantRanges    = &push_range;

    VkResult res = vkCreatePipelineLayout(g_render.device, &layout_ci, NULL,
                                          &g_render.water_pipeline_layout);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreatePipelineLayout (water) failed: %d\n", res);
        return MC_ERR_VULKAN;
    }

    /*
     * NOTE: Like the terrain pipeline, actual SPIR-V loading and full
     * graphics pipeline creation is deferred until shaders are compiled.
     * The pipeline handle remains VK_NULL_HANDLE for now.
     */
    g_render.water_pipeline = VK_NULL_HANDLE;
    return MC_OK;
}
