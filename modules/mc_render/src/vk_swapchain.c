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

/* ---- Depth buffer ---- */

static VkFormat find_supported_depth_format(void)
{
    VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    uint32_t count = sizeof(candidates) / sizeof(candidates[0]);

    for (uint32_t i = 0; i < count; i++) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(g_render.physical_device,
                                            candidates[i], &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return candidates[i];
        }
    }
    return VK_FORMAT_D32_SFLOAT; /* fallback */
}

mc_error_t vk_create_depth_resources(void)
{
    g_render.depth_format = find_supported_depth_format();

    /* Create depth image */
    VkImageCreateInfo img_ci;
    memset(&img_ci, 0, sizeof(img_ci));
    img_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_ci.imageType     = VK_IMAGE_TYPE_2D;
    img_ci.format        = g_render.depth_format;
    img_ci.extent.width  = g_render.swapchain_extent.width;
    img_ci.extent.height = g_render.swapchain_extent.height;
    img_ci.extent.depth  = 1;
    img_ci.mipLevels     = 1;
    img_ci.arrayLayers   = 1;
    img_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    img_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    img_ci.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    img_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    img_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult res = vkCreateImage(g_render.device, &img_ci, NULL, &g_render.depth_image);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateImage (depth) failed: %d\n", res);
        return MC_ERR_VULKAN;
    }

    /* Allocate memory for depth image */
    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(g_render.device, g_render.depth_image, &mem_req);

    uint32_t mem_type = vk_find_memory_type(mem_req.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mem_type == UINT32_MAX) {
        fprintf(stderr, "Failed to find suitable memory type for depth image\n");
        vkDestroyImage(g_render.device, g_render.depth_image, NULL);
        g_render.depth_image = VK_NULL_HANDLE;
        return MC_ERR_VULKAN;
    }

    VkMemoryAllocateInfo alloc;
    memset(&alloc, 0, sizeof(alloc));
    alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize  = mem_req.size;
    alloc.memoryTypeIndex = mem_type;

    res = vkAllocateMemory(g_render.device, &alloc, NULL, &g_render.depth_memory);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkAllocateMemory (depth) failed: %d\n", res);
        vkDestroyImage(g_render.device, g_render.depth_image, NULL);
        g_render.depth_image = VK_NULL_HANDLE;
        return MC_ERR_VULKAN;
    }

    vkBindImageMemory(g_render.device, g_render.depth_image, g_render.depth_memory, 0);

    /* Create depth image view */
    VkImageViewCreateInfo iv_ci;
    memset(&iv_ci, 0, sizeof(iv_ci));
    iv_ci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv_ci.image    = g_render.depth_image;
    iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv_ci.format   = g_render.depth_format;
    iv_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    iv_ci.subresourceRange.baseMipLevel   = 0;
    iv_ci.subresourceRange.levelCount     = 1;
    iv_ci.subresourceRange.baseArrayLayer = 0;
    iv_ci.subresourceRange.layerCount     = 1;

    res = vkCreateImageView(g_render.device, &iv_ci, NULL, &g_render.depth_image_view);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateImageView (depth) failed: %d\n", res);
        vkFreeMemory(g_render.device, g_render.depth_memory, NULL);
        vkDestroyImage(g_render.device, g_render.depth_image, NULL);
        g_render.depth_memory = VK_NULL_HANDLE;
        g_render.depth_image  = VK_NULL_HANDLE;
        return MC_ERR_VULKAN;
    }

    return MC_OK;
}

void vk_destroy_depth_resources(void)
{
    if (g_render.device == VK_NULL_HANDLE) return;

    if (g_render.depth_image_view) {
        vkDestroyImageView(g_render.device, g_render.depth_image_view, NULL);
        g_render.depth_image_view = VK_NULL_HANDLE;
    }
    if (g_render.depth_image) {
        vkDestroyImage(g_render.device, g_render.depth_image, NULL);
        g_render.depth_image = VK_NULL_HANDLE;
    }
    if (g_render.depth_memory) {
        vkFreeMemory(g_render.device, g_render.depth_memory, NULL);
        g_render.depth_memory = VK_NULL_HANDLE;
    }
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

    vk_destroy_depth_resources();

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
    VkAttachmentDescription attachments[2];
    memset(attachments, 0, sizeof(attachments));

    /* Color attachment (index 0) */
    attachments[0].format         = g_render.swapchain_format;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    /* Depth attachment (index 1) */
    attachments[1].format         = g_render.depth_format;
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_ref;
    color_ref.attachment = 0;
    color_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_ref;
    depth_ref.attachment = 1;
    depth_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass;
    memset(&subpass, 0, sizeof(subpass));
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_ref;
    subpass.pDepthStencilAttachment = &depth_ref;

    VkSubpassDependency dep;
    memset(&dep, 0, sizeof(dep));
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = 2;
    ci.pAttachments    = attachments;
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
        VkImageView fb_attachments[2] = {
            g_render.swapchain_image_views[i],
            g_render.depth_image_view
        };

        VkFramebufferCreateInfo ci;
        memset(&ci, 0, sizeof(ci));
        ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ci.renderPass      = g_render.render_pass;
        ci.attachmentCount = 2;
        ci.pAttachments    = fb_attachments;
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

static VkShaderModule load_shader(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open shader: %s\n", path);
        return VK_NULL_HANDLE;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint32_t *code = malloc((size_t)size);
    fread(code, 1, (size_t)size, f);
    fclose(f);

    VkShaderModuleCreateInfo ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = (size_t)size;
    ci.pCode    = code;

    VkShaderModule mod = VK_NULL_HANDLE;
    vkCreateShaderModule(g_render.device, &ci, NULL, &mod);
    free(code);
    return mod;
}

mc_error_t vk_create_pipeline(void)
{
    VkPushConstantRange push_range;
    memset(&push_range, 0, sizeof(push_range));
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset     = 0;
    push_range.size       = sizeof(float) * 16;

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

    VkShaderModule vert = load_shader("modules/mc_render/shaders/terrain.vert.spv");
    VkShaderModule frag = load_shader("modules/mc_render/shaders/terrain.frag.spv");
    if (!vert || !frag) {
        fprintf(stderr, "Failed to load terrain shaders\n");
        if (vert) vkDestroyShaderModule(g_render.device, vert, NULL);
        if (frag) vkDestroyShaderModule(g_render.device, frag, NULL);
        g_render.graphics_pipeline = VK_NULL_HANDLE;
        return MC_OK;
    }

    VkPipelineShaderStageCreateInfo stages[2];
    memset(stages, 0, sizeof(stages));
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName  = "main";

    /* Vertex input: 2 x uint32 per vertex (packed position + packed lighting) */
    VkVertexInputBindingDescription binding;
    memset(&binding, 0, sizeof(binding));
    binding.binding   = 0;
    binding.stride    = 8;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2];
    memset(attrs, 0, sizeof(attrs));
    attrs[0].location = 0;
    attrs[0].binding  = 0;
    attrs[0].format   = VK_FORMAT_R32_UINT;
    attrs[0].offset   = 0;
    attrs[1].location = 1;
    attrs[1].binding  = 0;
    attrs[1].format   = VK_FORMAT_R32_UINT;
    attrs[1].offset   = 4;

    VkPipelineVertexInputStateCreateInfo vertex_input;
    memset(&vertex_input, 0, sizeof(vertex_input));
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount   = 1;
    vertex_input.pVertexBindingDescriptions      = &binding;
    vertex_input.vertexAttributeDescriptionCount = 2;
    vertex_input.pVertexAttributeDescriptions    = attrs;

    VkPipelineInputAssemblyStateCreateInfo input_asm;
    memset(&input_asm, 0, sizeof(input_asm));
    input_asm.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = { 0, 0, (float)g_render.swapchain_extent.width,
                            (float)g_render.swapchain_extent.height, 0.0f, 1.0f };
    VkRect2D scissor = { {0, 0}, g_render.swapchain_extent };

    VkPipelineViewportStateCreateInfo viewport_state;
    memset(&viewport_state, 0, sizeof(viewport_state));
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = &viewport;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo raster;
    memset(&raster, 0, sizeof(raster));
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode    = VK_CULL_MODE_BACK_BIT;
    raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo msaa;
    memset(&msaa, 0, sizeof(msaa));
    msaa.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth;
    memset(&depth, 0, sizeof(depth));
    depth.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable  = VK_TRUE;
    depth.depthWriteEnable = VK_TRUE;
    depth.depthCompareOp   = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blend_att;
    memset(&blend_att, 0, sizeof(blend_att));
    blend_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend;
    memset(&blend, 0, sizeof(blend));
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blend_att;

    VkGraphicsPipelineCreateInfo pipeline_ci;
    memset(&pipeline_ci, 0, sizeof(pipeline_ci));
    pipeline_ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.stageCount          = 2;
    pipeline_ci.pStages             = stages;
    pipeline_ci.pVertexInputState   = &vertex_input;
    pipeline_ci.pInputAssemblyState = &input_asm;
    pipeline_ci.pViewportState      = &viewport_state;
    pipeline_ci.pRasterizationState = &raster;
    pipeline_ci.pMultisampleState   = &msaa;
    pipeline_ci.pDepthStencilState  = &depth;
    pipeline_ci.pColorBlendState    = &blend;
    pipeline_ci.layout              = g_render.pipeline_layout;
    pipeline_ci.renderPass          = g_render.render_pass;
    pipeline_ci.subpass             = 0;

    res = vkCreateGraphicsPipelines(g_render.device, VK_NULL_HANDLE, 1, &pipeline_ci,
                                    NULL, &g_render.graphics_pipeline);

    vkDestroyShaderModule(g_render.device, vert, NULL);
    vkDestroyShaderModule(g_render.device, frag, NULL);

    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateGraphicsPipelines failed: %d\n", res);
        return MC_ERR_VULKAN;
    }

    fprintf(stderr, "Graphics pipeline created successfully\n");
    return MC_OK;
}
