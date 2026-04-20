#ifndef RENDER_INTERNAL_H
#define RENDER_INTERNAL_H

#include "mc_types.h"
#include "mc_error.h"

#ifdef __APPLE__
  #define VK_USE_PLATFORM_MACOS_MVK
#endif
#include <vulkan/vulkan.h>

#include <stdint.h>

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_MESH_SLOTS       4096

typedef struct {
    VkBuffer       vertex_buffer;
    VkDeviceMemory vertex_memory;
    VkBuffer       index_buffer;
    VkDeviceMemory index_memory;
    uint32_t       index_count;
    uint32_t       vertex_count;
    uint8_t        in_use;
} mesh_slot_t;

typedef struct {
    /* Instance / device */
    VkInstance               instance;
    VkPhysicalDevice         physical_device;
    VkDevice                 device;
    VkQueue                  graphics_queue;
    uint32_t                 graphics_family;

    /* Swapchain */
    VkSurfaceKHR             surface;
    VkSwapchainKHR           swapchain;
    VkFormat                 swapchain_format;
    VkExtent2D               swapchain_extent;
    uint32_t                 image_count;
    VkImage*                 swapchain_images;
    VkImageView*             swapchain_image_views;

    /* Depth buffer */
    VkFormat                 depth_format;
    VkImage                  depth_image;
    VkDeviceMemory           depth_memory;
    VkImageView              depth_image_view;

    /* Render pass */
    VkRenderPass             render_pass;
    VkFramebuffer*           framebuffers;

    /* Command pool / buffers */
    VkCommandPool            command_pool;
    VkCommandBuffer          command_buffers[MAX_FRAMES_IN_FLIGHT];

    /* Synchronization */
    VkSemaphore              image_available[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore              render_finished[MAX_FRAMES_IN_FLIGHT];
    VkFence                  in_flight[MAX_FRAMES_IN_FLIGHT];
    uint32_t                 current_frame;

    /* Pipeline */
    VkPipelineLayout         pipeline_layout;
    VkPipeline               graphics_pipeline;

    /* Frame state */
    uint32_t                 current_image_index;
    VkCommandBuffer          active_cmd;
    mat4_t                   view_matrix;
    mat4_t                   projection_matrix;

    /* Clear color */
    float                    clear_r, clear_g, clear_b;

    /* Fog */
    float                    fog_start, fog_end;
    vec3_t                   fog_color;

    /* Mesh slots */
    mesh_slot_t              meshes[MAX_MESH_SLOTS];

    /* Memory properties */
    VkPhysicalDeviceMemoryProperties mem_props;

    /* Window dimensions */
    uint32_t                 width, height;
} render_state_t;

/* Global render state -- defined in vk_init.c */
extern render_state_t g_render;

/* Internal helpers */
mc_error_t vk_init_instance(void);
mc_error_t vk_select_physical_device(void);
mc_error_t vk_create_device(void);

mc_error_t vk_create_swapchain(void);
void       vk_destroy_swapchain(void);
mc_error_t vk_create_depth_resources(void);
void       vk_destroy_depth_resources(void);
mc_error_t vk_create_render_pass(void);
mc_error_t vk_create_framebuffers(void);

mc_error_t vk_create_command_pool(void);
mc_error_t vk_create_sync_objects(void);
mc_error_t vk_create_pipeline(void);

uint32_t   vk_find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

/* Mesh building -- returns a heap-allocated chunk_mesh_t (caller frees).
 * Returns NULL if section is empty or allocation fails. */
chunk_mesh_t *build_chunk_mesh_data(const chunk_section_t *section,
                                    chunk_pos_t chunk_pos, uint8_t section_y);

#endif /* RENDER_INTERNAL_H */
