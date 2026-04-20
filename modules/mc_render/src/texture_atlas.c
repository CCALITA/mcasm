#include "render_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Procedural texture atlas: 256x256 RGBA8 image arranged as a 16x16 grid
 * of 16x16 pixel tiles.  Each tile index maps to a block type's color.
 */

#define ATLAS_SIZE      256
#define TILE_SIZE       16
#define TILES_PER_ROW   16
#define ATLAS_PIXELS    (ATLAS_SIZE * ATLAS_SIZE)
#define BYTES_PER_PIXEL 4

typedef struct {
    uint8_t r, g, b, a;
} rgba_t;

/* Block-type tile colors (index = tile number in the atlas) */
static const rgba_t TILE_COLORS[] = {
    /* 0  AIR (transparent)      */ {   0,   0,   0,   0 },
    /* 1  STONE                  */ { 128, 128, 128, 255 },
    /* 2  GRASS_TOP              */ {   0, 200,   0, 255 },
    /* 3  GRASS_SIDE (brown strip at bottom, green above) -- filled procedurally */
    /*    base color green       */ {   0, 180,   0, 255 },
    /* 4  DIRT                   */ { 139,  90,  43, 255 },
    /* 5  COBBLESTONE            */ {  80,  80,  80, 255 },
    /* 6  SAND                   */ { 210, 190, 140, 255 },
    /* 7  WATER (semi-transparent) */ {  0,   0, 200, 128 },
    /* 8  OAK_LOG_SIDE           */ { 130, 100,  50, 255 },
    /* 9  OAK_LOG_TOP            */ { 160, 130,  70, 255 },
    /* 10 OAK_LEAVES             */ {  50, 140,  30, 200 },
    /* 11 BEDROCK                */ {  40,  40,  40, 255 },
    /* 12 GRAVEL                 */ { 120, 110, 110, 255 },
    /* 13 IRON_ORE               */ { 140, 130, 120, 255 },
    /* 14 COAL_ORE               */ { 100,  95,  95, 255 },
    /* 15 SNOW                   */ { 240, 240, 255, 255 },
};

#define TILE_COLOR_COUNT (sizeof(TILE_COLORS) / sizeof(TILE_COLORS[0]))

/*
 * Fill a single tile in the pixel buffer.
 * tile_index selects the 16x16 cell in row-major order.
 */
static void fill_tile(uint8_t *pixels, uint32_t tile_index, rgba_t color)
{
    uint32_t tile_x = tile_index % TILES_PER_ROW;
    uint32_t tile_y = tile_index / TILES_PER_ROW;
    uint32_t base_px = tile_x * TILE_SIZE;
    uint32_t base_py = tile_y * TILE_SIZE;

    for (uint32_t py = 0; py < TILE_SIZE; py++) {
        for (uint32_t px = 0; px < TILE_SIZE; px++) {
            uint32_t offset = ((base_py + py) * ATLAS_SIZE + (base_px + px)) * BYTES_PER_PIXEL;
            pixels[offset + 0] = color.r;
            pixels[offset + 1] = color.g;
            pixels[offset + 2] = color.b;
            pixels[offset + 3] = color.a;
        }
    }
}

/*
 * GRASS_SIDE tile (index 3): green top 12 rows, brown bottom 4 rows.
 */
static void fill_grass_side_tile(uint8_t *pixels)
{
    uint32_t tile_x = 3 % TILES_PER_ROW;
    uint32_t tile_y = 3 / TILES_PER_ROW;
    uint32_t base_px = tile_x * TILE_SIZE;
    uint32_t base_py = tile_y * TILE_SIZE;

    rgba_t green = {  0, 180,   0, 255 };
    rgba_t brown = { 139,  90,  43, 255 };

    for (uint32_t py = 0; py < TILE_SIZE; py++) {
        rgba_t c = (py >= 12) ? brown : green;
        for (uint32_t px = 0; px < TILE_SIZE; px++) {
            uint32_t offset = ((base_py + py) * ATLAS_SIZE + (base_px + px)) * BYTES_PER_PIXEL;
            pixels[offset + 0] = c.r;
            pixels[offset + 1] = c.g;
            pixels[offset + 2] = c.b;
            pixels[offset + 3] = c.a;
        }
    }
}

/*
 * Generate the full atlas pixel data (256x256 RGBA8).
 * Caller must free the returned pointer.
 */
static uint8_t *generate_atlas_pixels(void)
{
    uint32_t byte_count = ATLAS_PIXELS * BYTES_PER_PIXEL;
    uint8_t *pixels = calloc(1, byte_count);
    if (!pixels) return NULL;

    /* Fill known tiles with solid colors (skip tile 3: filled procedurally below) */
    for (uint32_t i = 0; i < TILE_COLOR_COUNT; i++) {
        if (i == 3) continue;
        fill_tile(pixels, i, TILE_COLORS[i]);
    }

    /* Fill remaining tiles with magenta (missing-texture indicator) */
    rgba_t magenta = { 255, 0, 255, 255 };
    for (uint32_t i = (uint32_t)TILE_COLOR_COUNT; i < TILES_PER_ROW * TILES_PER_ROW; i++) {
        fill_tile(pixels, i, magenta);
    }

    /* Tile 3: grass-side with green top / brown bottom strip */
    fill_grass_side_tile(pixels);

    return pixels;
}

/* ---- Vulkan upload helpers ---- */

static mc_error_t create_image(uint32_t width, uint32_t height,
                               VkFormat format, VkImageUsageFlags usage,
                               VkImage *image, VkDeviceMemory *memory)
{
    VkImageCreateInfo ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType     = VK_IMAGE_TYPE_2D;
    ci.format        = format;
    ci.extent.width  = width;
    ci.extent.height = height;
    ci.extent.depth  = 1;
    ci.mipLevels     = 1;
    ci.arrayLayers   = 1;
    ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ci.usage         = usage;
    ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult res = vkCreateImage(g_render.device, &ci, NULL, image);
    if (res != VK_SUCCESS) return MC_ERR_VULKAN;

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(g_render.device, *image, &req);

    VkMemoryAllocateInfo alloc;
    memset(&alloc, 0, sizeof(alloc));
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = req.size;
    alloc.memoryTypeIndex = vk_find_memory_type(req.memoryTypeBits,
                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (alloc.memoryTypeIndex == UINT32_MAX) {
        vkDestroyImage(g_render.device, *image, NULL);
        return MC_ERR_VULKAN;
    }

    res = vkAllocateMemory(g_render.device, &alloc, NULL, memory);
    if (res != VK_SUCCESS) {
        vkDestroyImage(g_render.device, *image, NULL);
        return MC_ERR_VULKAN;
    }

    vkBindImageMemory(g_render.device, *image, *memory, 0);
    return MC_OK;
}

/*
 * Record and submit a one-shot command buffer, then wait for it to complete.
 */
static VkCommandBuffer begin_one_shot_cmd(void)
{
    VkCommandBufferAllocateInfo ai;
    memset(&ai, 0, sizeof(ai));
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = g_render.command_pool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    VkCommandBuffer cmd;
    if (vkAllocateCommandBuffers(g_render.device, &ai, &cmd) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    VkCommandBufferBeginInfo bi;
    memset(&bi, 0, sizeof(bi));
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);

    return cmd;
}

static void end_one_shot_cmd(VkCommandBuffer cmd)
{
    vkEndCommandBuffer(cmd);

    VkSubmitInfo si;
    memset(&si, 0, sizeof(si));
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;

    vkQueueSubmit(g_render.graphics_queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(g_render.graphics_queue);

    vkFreeCommandBuffers(g_render.device, g_render.command_pool, 1, &cmd);
}

static void transition_image_layout(VkCommandBuffer cmd, VkImage image,
                                    VkImageLayout old_layout,
                                    VkImageLayout new_layout)
{
    VkImageMemoryBarrier barrier;
    memset(&barrier, 0, sizeof(barrier));
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = old_layout;
    barrier.newLayout                       = new_layout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0,
                         0, NULL, 0, NULL, 1, &barrier);
}

/* ---- Public API ---- */

mc_error_t vk_ensure_atlas_desc_layout(void)
{
    if (g_render.atlas_desc_layout) return MC_OK;

    VkDescriptorSetLayoutBinding binding;
    memset(&binding, 0, sizeof(binding));
    binding.binding         = 0;
    binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dsl_ci;
    memset(&dsl_ci, 0, sizeof(dsl_ci));
    dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.bindingCount = 1;
    dsl_ci.pBindings    = &binding;

    VkResult res = vkCreateDescriptorSetLayout(g_render.device, &dsl_ci, NULL,
                                               &g_render.atlas_desc_layout);
    return (res == VK_SUCCESS) ? MC_OK : MC_ERR_VULKAN;
}

mc_error_t vk_create_texture_atlas(void)
{
    if (!g_render.device || !g_render.command_pool) return MC_ERR_NOT_READY;

    /* Generate pixel data */
    uint8_t *pixels = generate_atlas_pixels();
    if (!pixels) return MC_ERR_OUT_OF_MEMORY;

    uint32_t byte_count = ATLAS_PIXELS * BYTES_PER_PIXEL;

    /* Create the device-local image */
    mc_error_t err = create_image(ATLAS_SIZE, ATLAS_SIZE,
                                  VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  &g_render.atlas_image, &g_render.atlas_memory);
    if (err != MC_OK) { free(pixels); return err; }

    /* Create staging buffer */
    VkBuffer staging_buf         = VK_NULL_HANDLE;
    VkDeviceMemory staging_mem   = VK_NULL_HANDLE;

    VkBufferCreateInfo buf_ci;
    memset(&buf_ci, 0, sizeof(buf_ci));
    buf_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_ci.size        = byte_count;
    buf_ci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(g_render.device, &buf_ci, NULL, &staging_buf) != VK_SUCCESS) {
        free(pixels);
        return MC_ERR_VULKAN;
    }

    VkMemoryRequirements buf_req;
    vkGetBufferMemoryRequirements(g_render.device, staging_buf, &buf_req);

    VkMemoryAllocateInfo buf_alloc;
    memset(&buf_alloc, 0, sizeof(buf_alloc));
    buf_alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    buf_alloc.allocationSize  = buf_req.size;
    buf_alloc.memoryTypeIndex = vk_find_memory_type(buf_req.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (buf_alloc.memoryTypeIndex == UINT32_MAX) {
        vkDestroyBuffer(g_render.device, staging_buf, NULL);
        free(pixels);
        return MC_ERR_VULKAN;
    }

    if (vkAllocateMemory(g_render.device, &buf_alloc, NULL, &staging_mem) != VK_SUCCESS) {
        vkDestroyBuffer(g_render.device, staging_buf, NULL);
        free(pixels);
        return MC_ERR_VULKAN;
    }

    vkBindBufferMemory(g_render.device, staging_buf, staging_mem, 0);

    /* Copy pixel data into staging buffer */
    void *mapped = NULL;
    vkMapMemory(g_render.device, staging_mem, 0, byte_count, 0, &mapped);
    memcpy(mapped, pixels, byte_count);
    vkUnmapMemory(g_render.device, staging_mem);
    free(pixels);

    /* Transfer: transition, copy, transition */
    VkCommandBuffer cmd = begin_one_shot_cmd();
    if (cmd == VK_NULL_HANDLE) {
        vkDestroyBuffer(g_render.device, staging_buf, NULL);
        vkFreeMemory(g_render.device, staging_mem, NULL);
        return MC_ERR_VULKAN;
    }

    transition_image_layout(cmd, g_render.atlas_image,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region;
    memset(&region, 0, sizeof(region));
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount     = 1;
    region.imageExtent.width               = ATLAS_SIZE;
    region.imageExtent.height              = ATLAS_SIZE;
    region.imageExtent.depth               = 1;

    vkCmdCopyBufferToImage(cmd, staging_buf, g_render.atlas_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    transition_image_layout(cmd, g_render.atlas_image,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    end_one_shot_cmd(cmd);

    /* Clean up staging resources */
    vkDestroyBuffer(g_render.device, staging_buf, NULL);
    vkFreeMemory(g_render.device, staging_mem, NULL);

    /* Create image view */
    VkImageViewCreateInfo iv_ci;
    memset(&iv_ci, 0, sizeof(iv_ci));
    iv_ci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv_ci.image    = g_render.atlas_image;
    iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv_ci.format   = VK_FORMAT_R8G8B8A8_UNORM;
    iv_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    iv_ci.subresourceRange.baseMipLevel   = 0;
    iv_ci.subresourceRange.levelCount     = 1;
    iv_ci.subresourceRange.baseArrayLayer = 0;
    iv_ci.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(g_render.device, &iv_ci, NULL, &g_render.atlas_view) != VK_SUCCESS)
        return MC_ERR_VULKAN;

    /* Create sampler */
    VkSamplerCreateInfo sam_ci;
    memset(&sam_ci, 0, sizeof(sam_ci));
    sam_ci.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sam_ci.magFilter    = VK_FILTER_NEAREST;
    sam_ci.minFilter    = VK_FILTER_NEAREST;
    sam_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sam_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sam_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sam_ci.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    if (vkCreateSampler(g_render.device, &sam_ci, NULL, &g_render.atlas_sampler) != VK_SUCCESS)
        return MC_ERR_VULKAN;

    mc_error_t layout_err = vk_ensure_atlas_desc_layout();
    if (layout_err != MC_OK) return layout_err;

    /* Create descriptor pool */
    VkDescriptorPoolSize pool_size;
    pool_size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo dp_ci;
    memset(&dp_ci, 0, sizeof(dp_ci));
    dp_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_ci.maxSets       = 1;
    dp_ci.poolSizeCount = 1;
    dp_ci.pPoolSizes    = &pool_size;

    if (vkCreateDescriptorPool(g_render.device, &dp_ci, NULL,
                               &g_render.atlas_desc_pool) != VK_SUCCESS)
        return MC_ERR_VULKAN;

    /* Allocate descriptor set */
    VkDescriptorSetAllocateInfo ds_ai;
    memset(&ds_ai, 0, sizeof(ds_ai));
    ds_ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ds_ai.descriptorPool     = g_render.atlas_desc_pool;
    ds_ai.descriptorSetCount = 1;
    ds_ai.pSetLayouts        = &g_render.atlas_desc_layout;

    if (vkAllocateDescriptorSets(g_render.device, &ds_ai,
                                 &g_render.atlas_desc_set) != VK_SUCCESS)
        return MC_ERR_VULKAN;

    /* Write descriptor */
    VkDescriptorImageInfo img_info;
    memset(&img_info, 0, sizeof(img_info));
    img_info.sampler     = g_render.atlas_sampler;
    img_info.imageView   = g_render.atlas_view;
    img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write;
    memset(&write, 0, sizeof(write));
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = g_render.atlas_desc_set;
    write.dstBinding      = 0;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo      = &img_info;

    vkUpdateDescriptorSets(g_render.device, 1, &write, 0, NULL);

    return MC_OK;
}

void vk_destroy_texture_atlas(void)
{
    if (!g_render.device) return;

    if (g_render.atlas_desc_pool)
        vkDestroyDescriptorPool(g_render.device, g_render.atlas_desc_pool, NULL);
    if (g_render.atlas_desc_layout)
        vkDestroyDescriptorSetLayout(g_render.device, g_render.atlas_desc_layout, NULL);
    if (g_render.atlas_sampler)
        vkDestroySampler(g_render.device, g_render.atlas_sampler, NULL);
    if (g_render.atlas_view)
        vkDestroyImageView(g_render.device, g_render.atlas_view, NULL);
    if (g_render.atlas_image)
        vkDestroyImage(g_render.device, g_render.atlas_image, NULL);
    if (g_render.atlas_memory)
        vkFreeMemory(g_render.device, g_render.atlas_memory, NULL);

    g_render.atlas_desc_pool   = VK_NULL_HANDLE;
    g_render.atlas_desc_layout = VK_NULL_HANDLE;
    g_render.atlas_desc_set    = VK_NULL_HANDLE;
    g_render.atlas_sampler     = VK_NULL_HANDLE;
    g_render.atlas_view        = VK_NULL_HANDLE;
    g_render.atlas_image       = VK_NULL_HANDLE;
    g_render.atlas_memory      = VK_NULL_HANDLE;
}
