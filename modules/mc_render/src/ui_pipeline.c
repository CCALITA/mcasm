/* ui_pipeline.c -- UI render pipeline: 2D screen-space quads
 *
 * Creates a separate Vulkan pipeline for screen-space UI rendering.
 *   - Vertex format matches ui_vertex_t: float x, y, u, v  (16 bytes)
 *   - Positions arrive in NDC [-1,1] from the UI module
 *   - No depth testing (UI is always on top)
 *   - Alpha blending enabled (src-alpha, one-minus-src-alpha)
 */

#include "render_internal.h"
#include "mc_ui.h"
#include <string.h>
#include <stdio.h>

/* Maximum UI vertices per frame (matches mc_ui MAX_UI_VERTS). */
#define UI_MAX_VERTS       (4096 * 6)
#define UI_VERTEX_SIZE     16  /* sizeof(float) * 4 : x, y, u, v */
#define UI_BUFFER_BYTES    (UI_MAX_VERTS * UI_VERTEX_SIZE)

/* ---- Pipeline creation -------------------------------------------------- */

mc_error_t vk_create_ui_pipeline(void)
{
    if (!g_render.device || !g_render.render_pass)
        return MC_ERR_NOT_READY;

    /* Pipeline layout -- no push constants, no descriptors */
    VkPipelineLayoutCreateInfo layout_ci;
    memset(&layout_ci, 0, sizeof(layout_ci));
    layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkResult res = vkCreatePipelineLayout(g_render.device, &layout_ci, NULL,
                                          &g_render.ui_pipeline_layout);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreatePipelineLayout (UI) failed: %d\n", res);
        return MC_ERR_VULKAN;
    }

    /*
     * Full pipeline creation requires compiled SPIR-V shaders for
     * ui.vert and ui.frag.  The pipeline handle stays VK_NULL_HANDLE
     * and vk_draw_ui() is a safe no-op until shaders are loaded.
     */
    g_render.ui_pipeline = VK_NULL_HANDLE;

    /* Pre-allocate a host-visible vertex buffer for UI quads. */
    VkMemoryPropertyFlags host_visible = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    mc_error_t err = vk_create_buffer(UI_BUFFER_BYTES,
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      host_visible,
                                      &g_render.ui_vertex_buffer,
                                      &g_render.ui_vertex_memory);
    if (err != MC_OK) return err;

    g_render.ui_vertex_capacity = UI_BUFFER_BYTES;
    return MC_OK;
}

/* ---- Cleanup ------------------------------------------------------------ */

void vk_destroy_ui_pipeline(void)
{
    if (!g_render.device) return;

    if (g_render.ui_vertex_buffer) {
        vkDestroyBuffer(g_render.device, g_render.ui_vertex_buffer, NULL);
        g_render.ui_vertex_buffer = VK_NULL_HANDLE;
    }
    if (g_render.ui_vertex_memory) {
        vkFreeMemory(g_render.device, g_render.ui_vertex_memory, NULL);
        g_render.ui_vertex_memory = VK_NULL_HANDLE;
    }
    if (g_render.ui_pipeline) {
        vkDestroyPipeline(g_render.device, g_render.ui_pipeline, NULL);
        g_render.ui_pipeline = VK_NULL_HANDLE;
    }
    if (g_render.ui_pipeline_layout) {
        vkDestroyPipelineLayout(g_render.device, g_render.ui_pipeline_layout, NULL);
        g_render.ui_pipeline_layout = VK_NULL_HANDLE;
    }
}

/* ---- Draw --------------------------------------------------------------- */

void vk_draw_ui(VkCommandBuffer cmd)
{
    if (!cmd || !g_render.ui_vertex_buffer)
        return;

    void *mapped = NULL;
    VkResult res = vkMapMemory(g_render.device, g_render.ui_vertex_memory,
                               0, g_render.ui_vertex_capacity, 0, &mapped);
    if (res != VK_SUCCESS) return;

    uint32_t vert_count = mc_ui_get_vertices(mapped, g_render.ui_vertex_capacity);
    vkUnmapMemory(g_render.device, g_render.ui_vertex_memory);

    mc_ui_render();

    if (vert_count == 0) return;

    if (g_render.ui_pipeline) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_render.ui_pipeline);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &g_render.ui_vertex_buffer, &offset);
        vkCmdDraw(cmd, vert_count, 1, 0, 0);
    }
}
