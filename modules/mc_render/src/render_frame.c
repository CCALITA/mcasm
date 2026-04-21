#include "render_internal.h"
#include "mc_render.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---- Init / shutdown ---- */

mc_error_t mc_render_init(void *window_handle, uint32_t width, uint32_t height)
{
    memset(&g_render, 0, sizeof(g_render));
    g_render.width   = width;
    g_render.height  = height;
    g_render.clear_r = 0.53f; /* sky blue */
    g_render.clear_g = 0.81f;
    g_render.clear_b = 0.92f;

    mc_error_t err;

    err = vk_init_instance();
    if (err != MC_OK) return err;

    err = vk_select_physical_device();
    if (err != MC_OK) return err;

    err = vk_create_device();
    if (err != MC_OK) return err;

    /*
     * Surface creation requires a platform window handle (GLFW etc).
     * If no window is provided, skip swapchain setup -- useful for headless
     * testing or mesh-only workflows.
     */
    if (window_handle) {
        /* The caller provides a VkSurfaceKHR cast to void* */
        g_render.surface = (VkSurfaceKHR)(uintptr_t)window_handle;

        err = vk_create_swapchain();
        if (err != MC_OK) return err;

        err = vk_create_depth_resources();
        if (err != MC_OK) return err;

        err = vk_create_render_pass();
        if (err != MC_OK) return err;

        err = vk_create_framebuffers();
        if (err != MC_OK) return err;

        err = vk_create_command_pool();
        if (err != MC_OK) return err;

        err = vk_create_sync_objects();
        if (err != MC_OK) return err;

        err = vk_create_pipeline();
        if (err != MC_OK) return err;

        err = vk_create_texture_atlas();
        if (err != MC_OK) return err;
    }

    return MC_OK;
}

void mc_render_shutdown(void)
{
    if (g_render.device) {
        vkDeviceWaitIdle(g_render.device);
    }

    vk_destroy_texture_atlas();

    /* Free entity vertex buffer */
    if (g_render.entity_vertex_buffer)
        vkDestroyBuffer(g_render.device, g_render.entity_vertex_buffer, NULL);
    if (g_render.entity_vertex_memory)
        vkFreeMemory(g_render.device, g_render.entity_vertex_memory, NULL);

    for (uint32_t i = 0; i < MAX_MESH_SLOTS; i++) {
        if (g_render.meshes[i].in_use) {
            mc_render_free_mesh((uint64_t)(i + 1));
        }
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (g_render.image_available[i])
            vkDestroySemaphore(g_render.device, g_render.image_available[i], NULL);
        if (g_render.render_finished[i])
            vkDestroySemaphore(g_render.device, g_render.render_finished[i], NULL);
        if (g_render.in_flight[i])
            vkDestroyFence(g_render.device, g_render.in_flight[i], NULL);
    }

    if (g_render.command_pool)
        vkDestroyCommandPool(g_render.device, g_render.command_pool, NULL);

    if (g_render.graphics_pipeline)
        vkDestroyPipeline(g_render.device, g_render.graphics_pipeline, NULL);

    if (g_render.pipeline_layout)
        vkDestroyPipelineLayout(g_render.device, g_render.pipeline_layout, NULL);

    if (g_render.render_pass)
        vkDestroyRenderPass(g_render.device, g_render.render_pass, NULL);

    vk_destroy_swapchain();

    if (g_render.device)
        vkDestroyDevice(g_render.device, NULL);

    /* Note: surface is owned by the platform layer, not destroyed here */

    if (g_render.instance)
        vkDestroyInstance(g_render.instance, NULL);

    memset(&g_render, 0, sizeof(g_render));
}

mc_error_t mc_render_resize(uint32_t width, uint32_t height)
{
    if (!g_render.device || !g_render.surface) return MC_ERR_NOT_READY;

    vkDeviceWaitIdle(g_render.device);

    g_render.width  = width;
    g_render.height = height;

    vk_destroy_swapchain();

    mc_error_t err;
    err = vk_create_swapchain();
    if (err != MC_OK) return err;
    err = vk_create_depth_resources();
    if (err != MC_OK) return err;
    err = vk_create_framebuffers();
    if (err != MC_OK) return err;

    return MC_OK;
}

/* ---- Mesh management ---- */

static uint32_t find_free_slot(void)
{
    for (uint32_t i = 0; i < MAX_MESH_SLOTS; i++) {
        if (!g_render.meshes[i].in_use) return i;
    }
    return UINT32_MAX;
}

static mc_error_t create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                VkMemoryPropertyFlags props,
                                VkBuffer *buffer, VkDeviceMemory *memory)
{
    VkBufferCreateInfo ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size        = size;
    ci.usage       = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult res = vkCreateBuffer(g_render.device, &ci, NULL, buffer);
    if (res != VK_SUCCESS) return MC_ERR_VULKAN;

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(g_render.device, *buffer, &mem_req);

    VkMemoryAllocateInfo alloc;
    memset(&alloc, 0, sizeof(alloc));
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = mem_req.size;
    alloc.memoryTypeIndex = vk_find_memory_type(mem_req.memoryTypeBits, props);
    if (alloc.memoryTypeIndex == UINT32_MAX) {
        vkDestroyBuffer(g_render.device, *buffer, NULL);
        return MC_ERR_VULKAN;
    }

    res = vkAllocateMemory(g_render.device, &alloc, NULL, memory);
    if (res != VK_SUCCESS) {
        vkDestroyBuffer(g_render.device, *buffer, NULL);
        return MC_ERR_VULKAN;
    }

    vkBindBufferMemory(g_render.device, *buffer, *memory, 0);
    return MC_OK;
}

uint64_t mc_render_upload_mesh(const chunk_mesh_t *mesh)
{
    if (!mesh || mesh->vertex_count == 0) return 0;
    if (!g_render.device) return 0;

    uint32_t slot = find_free_slot();
    if (slot == UINT32_MAX) return 0;

    mesh_slot_t *ms = &g_render.meshes[slot];
    VkMemoryPropertyFlags host_visible = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    /* Vertex buffer */
    VkDeviceSize vb_size = mesh->vertex_count * sizeof(chunk_vertex_t);
    mc_error_t err = create_buffer(vb_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   host_visible, &ms->vertex_buffer, &ms->vertex_memory);
    if (err != MC_OK) return 0;

    void *mapped = NULL;
    vkMapMemory(g_render.device, ms->vertex_memory, 0, vb_size, 0, &mapped);
    memcpy(mapped, mesh->vertices, vb_size);
    vkUnmapMemory(g_render.device, ms->vertex_memory);

    /* Index buffer */
    if (mesh->index_count > 0 && mesh->indices) {
        VkDeviceSize ib_size = mesh->index_count * sizeof(uint32_t);
        err = create_buffer(ib_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                            host_visible, &ms->index_buffer, &ms->index_memory);
        if (err != MC_OK) {
            vkDestroyBuffer(g_render.device, ms->vertex_buffer, NULL);
            vkFreeMemory(g_render.device, ms->vertex_memory, NULL);
            memset(ms, 0, sizeof(*ms));
            return 0;
        }

        vkMapMemory(g_render.device, ms->index_memory, 0, ib_size, 0, &mapped);
        memcpy(mapped, mesh->indices, ib_size);
        vkUnmapMemory(g_render.device, ms->index_memory);
        ms->index_count = mesh->index_count;
    }

    ms->vertex_count = mesh->vertex_count;
    ms->in_use       = 1;

    return (uint64_t)(slot + 1); /* 1-based handle */
}

void mc_render_free_mesh(uint64_t mesh_handle)
{
    if (mesh_handle == 0 || mesh_handle > MAX_MESH_SLOTS) return;
    uint32_t slot = (uint32_t)(mesh_handle - 1);
    mesh_slot_t *ms = &g_render.meshes[slot];
    if (!ms->in_use) return;

    if (g_render.device) {
        if (ms->index_buffer)  vkDestroyBuffer(g_render.device, ms->index_buffer, NULL);
        if (ms->index_memory)  vkFreeMemory(g_render.device, ms->index_memory, NULL);
        if (ms->vertex_buffer) vkDestroyBuffer(g_render.device, ms->vertex_buffer, NULL);
        if (ms->vertex_memory) vkFreeMemory(g_render.device, ms->vertex_memory, NULL);
    }
    memset(ms, 0, sizeof(*ms));
}

void mc_render_update_mesh(uint64_t mesh_handle, const chunk_mesh_t *mesh)
{
    mc_render_free_mesh(mesh_handle);
    /* Re-upload to the same slot is not guaranteed; caller should use return value */
    (void)mc_render_upload_mesh(mesh);
}

/* ---- Frame rendering ---- */

/* Compute MVP = projection * view (column-major 4x4 multiply) */
static mat4_t compute_vp_matrix(void)
{
    mat4_t mvp;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += g_render.projection_matrix.m[k * 4 + r] *
                       g_render.view_matrix.m[c * 4 + k];
            }
            mvp.m[c * 4 + r] = sum;
        }
    }
    return mvp;
}

void mc_render_begin_frame(const mat4_t *view, const mat4_t *projection)
{
    if (!g_render.device || !g_render.swapchain) return;

    g_render.view_matrix       = *view;
    g_render.projection_matrix = *projection;

    uint32_t frame = g_render.current_frame;
    vkWaitForFences(g_render.device, 1, &g_render.in_flight[frame], VK_TRUE, UINT64_MAX);
    vkResetFences(g_render.device, 1, &g_render.in_flight[frame]);

    VkResult res = vkAcquireNextImageKHR(g_render.device, g_render.swapchain, UINT64_MAX,
                                         g_render.image_available[frame], VK_NULL_HANDLE,
                                         &g_render.current_image_index);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        mc_render_resize(g_render.width, g_render.height);
        return;
    }

    VkCommandBuffer cmd = g_render.command_buffers[frame];
    g_render.active_cmd = cmd;

    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info;
    memset(&begin_info, 0, sizeof(begin_info));
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &begin_info);

    VkClearValue clear_values[2];
    memset(clear_values, 0, sizeof(clear_values));
    clear_values[0].color.float32[0] = g_render.clear_r;
    clear_values[0].color.float32[1] = g_render.clear_g;
    clear_values[0].color.float32[2] = g_render.clear_b;
    clear_values[0].color.float32[3] = 1.0f;
    clear_values[1].depthStencil.depth   = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo rp_info;
    memset(&rp_info, 0, sizeof(rp_info));
    rp_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_info.renderPass        = g_render.render_pass;
    rp_info.framebuffer       = g_render.framebuffers[g_render.current_image_index];
    rp_info.renderArea.offset = (VkOffset2D){0, 0};
    rp_info.renderArea.extent = g_render.swapchain_extent;
    rp_info.clearValueCount   = 2;
    rp_info.pClearValues      = clear_values;

    vkCmdBeginRenderPass(cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
}

void mc_render_draw_terrain(const uint64_t *mesh_handles, uint32_t count)
{
    if (!g_render.active_cmd || !g_render.graphics_pipeline) return;
    if (!mesh_handles || count == 0) return;

    vkCmdBindPipeline(g_render.active_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      g_render.graphics_pipeline);

    /* Bind texture atlas descriptor set */
    if (g_render.atlas_desc_set) {
        vkCmdBindDescriptorSets(g_render.active_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                g_render.pipeline_layout, 0, 1,
                                &g_render.atlas_desc_set, 0, NULL);
    }

    mat4_t mvp = compute_vp_matrix();

    vkCmdPushConstants(g_render.active_cmd, g_render.pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvp), &mvp);

    for (uint32_t i = 0; i < count; i++) {
        uint64_t h = mesh_handles[i];
        if (h == 0 || h > MAX_MESH_SLOTS) continue;
        mesh_slot_t *ms = &g_render.meshes[h - 1];
        if (!ms->in_use) continue;

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(g_render.active_cmd, 0, 1, &ms->vertex_buffer, &offset);

        if (ms->index_count > 0 && ms->index_buffer) {
            vkCmdBindIndexBuffer(g_render.active_cmd, ms->index_buffer, 0,
                                 VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(g_render.active_cmd, ms->index_count, 1, 0, 0, 0);
        } else {
            vkCmdDraw(g_render.active_cmd, ms->vertex_count, 1, 0, 0);
        }
    }
}

/* ---- Entity cube helper ---- */

/* Vertices per cube: 6 faces * 2 triangles * 3 verts = 36 */
#define ENTITY_VERTS_PER_CUBE 36

/*
 * Generate 36 vertices for a unit cube at the integer block position nearest
 * to `pos`, using the same packed chunk_vertex_t format as terrain.
 * Light is set to maximum (15) so entities are always bright.
 */
static void generate_entity_cube(const vec3_t *pos, chunk_vertex_t *out)
{
    /*
     * Clamp to the valid range for the packed position field
     * (x: 0..30, y: 0..510, z: 0..30 so the +1 corner still fits).
     */
    int bx = (int)(pos->x);
    int by = (int)(pos->y);
    int bz = (int)(pos->z);
    if (bx < 0)   bx = 0;
    if (by < 0)   by = 0;
    if (bz < 0)   bz = 0;
    if (bx > 30)  bx = 30;
    if (by > 510) by = 510;
    if (bz > 30)  bz = 30;

    uint32_t x0 = (uint32_t)bx;
    uint32_t y0 = (uint32_t)by;
    uint32_t z0 = (uint32_t)bz;
    uint32_t x1 = x0 + 1;
    uint32_t y1 = y0 + 1;
    uint32_t z1 = z0 + 1;

    #define PK(px, py, pz) \
        (((px) & 0x1Fu) | (((py) & 0x1FFu) << 5) | (((pz) & 0x1Fu) << 14))

    uint32_t c000 = PK(x0, y0, z0);
    uint32_t c100 = PK(x1, y0, z0);
    uint32_t c010 = PK(x0, y1, z0);
    uint32_t c110 = PK(x1, y1, z0);
    uint32_t c001 = PK(x0, y0, z1);
    uint32_t c101 = PK(x1, y0, z1);
    uint32_t c011 = PK(x0, y1, z1);
    uint32_t c111 = PK(x1, y1, z1);

    #undef PK

    /* 6 faces, 2 triangles (6 vertices) each, CCW winding */
    const uint32_t face_verts[6][6] = {
        /* +X */ {c100, c110, c111, c100, c111, c101},
        /* -X */ {c001, c011, c010, c001, c010, c000},
        /* +Y */ {c010, c011, c111, c010, c111, c110},
        /* -Y */ {c001, c000, c100, c001, c100, c101},
        /* +Z */ {c101, c111, c011, c101, c011, c001},
        /* -Z */ {c000, c010, c110, c000, c110, c100},
    };

    uint32_t vi = 0;
    for (int f = 0; f < 6; f++) {
        for (int v = 0; v < 6; v++) {
            out[vi].position_packed = face_verts[f][v];
            out[vi].lighting_packed = 15u;
            vi++;
        }
    }
}

void mc_render_draw_entities(const vec3_t *positions, const uint8_t *types, uint32_t count)
{
    if (!g_render.active_cmd || !g_render.graphics_pipeline) return;
    if (!positions || !types || count == 0) return;

    (void)types; /* color differentiation deferred until entity shader exists */

    VkDeviceSize needed = (VkDeviceSize)count * ENTITY_VERTS_PER_CUBE * sizeof(chunk_vertex_t);

    /* Grow the entity vertex buffer when the current one is too small */
    if (needed > g_render.entity_vertex_capacity) {
        if (g_render.entity_vertex_buffer) {
            vkDestroyBuffer(g_render.device, g_render.entity_vertex_buffer, NULL);
            g_render.entity_vertex_buffer = VK_NULL_HANDLE;
        }
        if (g_render.entity_vertex_memory) {
            vkFreeMemory(g_render.device, g_render.entity_vertex_memory, NULL);
            g_render.entity_vertex_memory = VK_NULL_HANDLE;
        }
        g_render.entity_vertex_capacity = 0;

        mc_error_t err = create_buffer(
            needed, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &g_render.entity_vertex_buffer, &g_render.entity_vertex_memory);
        if (err != MC_OK) {
            g_render.entity_vertex_buffer = VK_NULL_HANDLE;
            g_render.entity_vertex_memory = VK_NULL_HANDLE;
            return;
        }
        g_render.entity_vertex_capacity = needed;
    }

    /* Upload vertex data for all entity cubes */
    void *mapped = NULL;
    if (vkMapMemory(g_render.device, g_render.entity_vertex_memory, 0, needed, 0, &mapped) != VK_SUCCESS)
        return;

    chunk_vertex_t *verts = (chunk_vertex_t *)mapped;
    for (uint32_t i = 0; i < count; i++) {
        generate_entity_cube(&positions[i], &verts[i * ENTITY_VERTS_PER_CUBE]);
    }

    vkUnmapMemory(g_render.device, g_render.entity_vertex_memory);

    /* Bind pipeline and push VP matrix */
    vkCmdBindPipeline(g_render.active_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      g_render.graphics_pipeline);

    mat4_t mvp = compute_vp_matrix();

    vkCmdPushConstants(g_render.active_cmd, g_render.pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvp), &mvp);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(g_render.active_cmd, 0, 1,
                           &g_render.entity_vertex_buffer, &offset);
    vkCmdDraw(g_render.active_cmd, count * ENTITY_VERTS_PER_CUBE, 1, 0, 0);
}

void mc_render_draw_ui(void)
{
    /* No-op: UI rendering not yet implemented */
}

void mc_render_end_frame(void)
{
    if (!g_render.active_cmd) return;

    vkCmdEndRenderPass(g_render.active_cmd);
    vkEndCommandBuffer(g_render.active_cmd);

    uint32_t frame = g_render.current_frame;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit;
    memset(&submit, 0, sizeof(submit));
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &g_render.image_available[frame];
    submit.pWaitDstStageMask    = &wait_stage;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &g_render.active_cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &g_render.render_finished[frame];

    vkQueueSubmit(g_render.graphics_queue, 1, &submit, g_render.in_flight[frame]);

    VkPresentInfoKHR present;
    memset(&present, 0, sizeof(present));
    present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores    = &g_render.render_finished[frame];
    present.swapchainCount     = 1;
    present.pSwapchains        = &g_render.swapchain;
    present.pImageIndices      = &g_render.current_image_index;

    vkQueuePresentKHR(g_render.graphics_queue, &present);

    g_render.current_frame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
    g_render.active_cmd    = VK_NULL_HANDLE;
}

/* ---- Settings ---- */

void mc_render_set_clear_color(float r, float g, float b)
{
    g_render.clear_r = r;
    g_render.clear_g = g;
    g_render.clear_b = b;
}

void mc_render_set_fog(float start, float end, vec3_t color)
{
    g_render.fog_start = start;
    g_render.fog_end   = end;
    g_render.fog_color = color;
}
