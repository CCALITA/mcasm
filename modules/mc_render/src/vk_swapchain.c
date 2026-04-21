#include "render_internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

mc_error_t vk_create_swapchain(void)
{
    /* Query surface capabilities */
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_render.physical_device, g_render.surface, &caps);

    /* Choose format: prefer B8G8R8A8_SRGB */
    uint32_t fmt_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_render.physical_device, g_render.surface, &fmt_count, NULL);
    VkSurfaceFormatKHR *formats = calloc(fmt_count, sizeof(VkSurfaceFormatKHR));
    if (!formats) return MC_ERR_OUT_OF_MEMORY;
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_render.physical_device, g_render.surface, &fmt_count, formats);

    VkSurfaceFormatKHR chosen = formats[0];
    for (uint32_t i = 0; i < fmt_count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = formats[i];
            break;
        }
    }
    free(formats);

    g_render.swapchain_format = chosen.format;

    /* Choose extent */
    if (caps.currentExtent.width != UINT32_MAX) {
        g_render.swapchain_extent = caps.currentExtent;
    } else {
        g_render.swapchain_extent.width  = g_render.width;
        g_render.swapchain_extent.height = g_render.height;
    }

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
        image_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = g_render.surface;
    ci.minImageCount    = image_count;
    ci.imageFormat      = chosen.format;
    ci.imageColorSpace  = chosen.colorSpace;
    ci.imageExtent      = g_render.swapchain_extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform     = caps.currentTransform;
    ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode      = VK_PRESENT_MODE_FIFO_KHR;
    ci.clipped          = VK_TRUE;
    ci.oldSwapchain     = VK_NULL_HANDLE;

    VkResult res = vkCreateSwapchainKHR(g_render.device, &ci, NULL, &g_render.swapchain);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateSwapchainKHR failed: %d\n", res);
        return MC_ERR_VULKAN;
    }

    /* Get swapchain images */
    vkGetSwapchainImagesKHR(g_render.device, g_render.swapchain, &g_render.image_count, NULL);
    g_render.swapchain_images = calloc(g_render.image_count, sizeof(VkImage));
    if (!g_render.swapchain_images) return MC_ERR_OUT_OF_MEMORY;
    vkGetSwapchainImagesKHR(g_render.device, g_render.swapchain,
                            &g_render.image_count, g_render.swapchain_images);

    /* Create image views */
    g_render.swapchain_image_views = calloc(g_render.image_count, sizeof(VkImageView));
    if (!g_render.swapchain_image_views) return MC_ERR_OUT_OF_MEMORY;

    for (uint32_t i = 0; i < g_render.image_count; i++) {
        VkImageViewCreateInfo iv_ci;
        memset(&iv_ci, 0, sizeof(iv_ci));
        iv_ci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv_ci.image    = g_render.swapchain_images[i];
        iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv_ci.format   = g_render.swapchain_format;
        iv_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        iv_ci.subresourceRange.baseMipLevel   = 0;
        iv_ci.subresourceRange.levelCount     = 1;
        iv_ci.subresourceRange.baseArrayLayer = 0;
        iv_ci.subresourceRange.layerCount     = 1;

        res = vkCreateImageView(g_render.device, &iv_ci, NULL,
                                &g_render.swapchain_image_views[i]);
        if (res != VK_SUCCESS) {
            fprintf(stderr, "vkCreateImageView failed: %d\n", res);
            return MC_ERR_VULKAN;
        }
    }

    return MC_OK;
}

void vk_destroy_swapchain(void)
{
    if (g_render.device == VK_NULL_HANDLE) return;

    if (g_render.framebuffers) {
        for (uint32_t i = 0; i < g_render.image_count; i++) {
            if (g_render.framebuffers[i]) {
                vkDestroyFramebuffer(g_render.device, g_render.framebuffers[i], NULL);
            }
        }
        free(g_render.framebuffers);
        g_render.framebuffers = NULL;
    }

    if (g_render.swapchain_image_views) {
        for (uint32_t i = 0; i < g_render.image_count; i++) {
            if (g_render.swapchain_image_views[i]) {
                vkDestroyImageView(g_render.device, g_render.swapchain_image_views[i], NULL);
            }
        }
        free(g_render.swapchain_image_views);
        g_render.swapchain_image_views = NULL;
    }

    free(g_render.swapchain_images);
    g_render.swapchain_images = NULL;

    if (g_render.swapchain) {
        vkDestroySwapchainKHR(g_render.device, g_render.swapchain, NULL);
        g_render.swapchain = VK_NULL_HANDLE;
    }
}

mc_error_t vk_create_render_pass(void)
{
    VkAttachmentDescription color_attachment;
    memset(&color_attachment, 0, sizeof(color_attachment));
    color_attachment.format         = g_render.swapchain_format;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref;
    color_ref.attachment = 0;
    color_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass;
    memset(&subpass, 0, sizeof(subpass));
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &color_ref;

    VkSubpassDependency dep;
    memset(&dep, 0, sizeof(dep));
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = 1;
    ci.pAttachments    = &color_attachment;
    ci.subpassCount    = 1;
    ci.pSubpasses      = &subpass;
    ci.dependencyCount = 1;
    ci.pDependencies   = &dep;

    VkResult res = vkCreateRenderPass(g_render.device, &ci, NULL, &g_render.render_pass);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateRenderPass failed: %d\n", res);
        return MC_ERR_VULKAN;
    }
    return MC_OK;
}

mc_error_t vk_create_framebuffers(void)
{
    g_render.framebuffers = calloc(g_render.image_count, sizeof(VkFramebuffer));
    if (!g_render.framebuffers) return MC_ERR_OUT_OF_MEMORY;

    for (uint32_t i = 0; i < g_render.image_count; i++) {
        VkFramebufferCreateInfo ci;
        memset(&ci, 0, sizeof(ci));
        ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ci.renderPass      = g_render.render_pass;
        ci.attachmentCount = 1;
        ci.pAttachments    = &g_render.swapchain_image_views[i];
        ci.width           = g_render.swapchain_extent.width;
        ci.height          = g_render.swapchain_extent.height;
        ci.layers          = 1;

        VkResult res = vkCreateFramebuffer(g_render.device, &ci, NULL,
                                           &g_render.framebuffers[i]);
        if (res != VK_SUCCESS) {
            fprintf(stderr, "vkCreateFramebuffer failed: %d\n", res);
            return MC_ERR_VULKAN;
        }
    }
    return MC_OK;
}

mc_error_t vk_create_command_pool(void)
{
    VkCommandPoolCreateInfo ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = g_render.graphics_family;

    VkResult res = vkCreateCommandPool(g_render.device, &ci, NULL, &g_render.command_pool);
    if (res != VK_SUCCESS) return MC_ERR_VULKAN;

    VkCommandBufferAllocateInfo alloc;
    memset(&alloc, 0, sizeof(alloc));
    alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool        = g_render.command_pool;
    alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    res = vkAllocateCommandBuffers(g_render.device, &alloc, g_render.command_buffers);
    if (res != VK_SUCCESS) return MC_ERR_VULKAN;

    return MC_OK;
}

mc_error_t vk_create_sync_objects(void)
{
    VkSemaphoreCreateInfo sem_ci;
    memset(&sem_ci, 0, sizeof(sem_ci));
    sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_ci;
    memset(&fence_ci, 0, sizeof(fence_ci));
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(g_render.device, &sem_ci, NULL, &g_render.image_available[i]) != VK_SUCCESS ||
            vkCreateSemaphore(g_render.device, &sem_ci, NULL, &g_render.render_finished[i]) != VK_SUCCESS ||
            vkCreateFence(g_render.device, &fence_ci, NULL, &g_render.in_flight[i]) != VK_SUCCESS) {
            return MC_ERR_VULKAN;
        }
    }
    return MC_OK;
}

mc_error_t vk_create_pipeline(void)
{
    /* Pipeline layout with push constant for MVP matrix */
    VkPushConstantRange push_range;
    memset(&push_range, 0, sizeof(push_range));
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset     = 0;
    push_range.size       = sizeof(float) * 16; /* mat4 */

    /*
     * Ensure the descriptor set layout for the texture atlas sampler exists
     * before building the pipeline layout.
     */
    mc_error_t dsl_err = vk_ensure_atlas_desc_layout();
    if (dsl_err != MC_OK) return dsl_err;

    VkPipelineLayoutCreateInfo layout_ci;
    memset(&layout_ci, 0, sizeof(layout_ci));
    layout_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_ci.setLayoutCount         = 1;
    layout_ci.pSetLayouts            = &g_render.atlas_desc_layout;
    layout_ci.pushConstantRangeCount = 1;
    layout_ci.pPushConstantRanges    = &push_range;

    VkResult res = vkCreatePipelineLayout(g_render.device, &layout_ci, NULL,
                                          &g_render.pipeline_layout);
    if (res != VK_SUCCESS) return MC_ERR_VULKAN;

    /*
     * NOTE: The graphics pipeline requires compiled SPIR-V shaders.
     * Pipeline creation is deferred until shaders are loaded.
     * For now, the pipeline handle remains VK_NULL_HANDLE and
     * draw calls are no-ops when no pipeline is bound.
     */
    g_render.graphics_pipeline = VK_NULL_HANDLE;
    return MC_OK;
}
