#include "render_internal.h"
#include <string.h>
#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

render_state_t g_render;

mc_error_t vk_init_instance(void)
{
    VkApplicationInfo app_info;
    memset(&app_info, 0, sizeof(app_info));
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "mcasm";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName        = "mcasm-engine";
    app_info.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion         = VK_API_VERSION_1_0;

    /* Get required extensions from GLFW (includes surface + platform surface) */
    uint32_t glfw_ext_count = 0;
    const char **glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    const char *extensions[32];
    uint32_t ext_count = 0;

    for (uint32_t i = 0; i < glfw_ext_count && ext_count < 32; i++)
        extensions[ext_count++] = glfw_exts[i];

#ifdef __APPLE__
    /* MoltenVK needs portability enumeration beyond what GLFW requests */
    if (ext_count < 32)
        extensions[ext_count++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
#endif

    VkInstanceCreateInfo ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo        = &app_info;
    ci.enabledExtensionCount   = ext_count;
    ci.ppEnabledExtensionNames = extensions;
#ifdef __APPLE__
    ci.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkResult res = vkCreateInstance(&ci, NULL, &g_render.instance);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed: %d\n", res);
        return MC_ERR_VULKAN;
    }
    return MC_OK;
}

mc_error_t vk_create_surface(void *window_handle)
{
    VkResult res = glfwCreateWindowSurface(g_render.instance,
                                           (GLFWwindow *)window_handle,
                                           NULL, &g_render.surface);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "glfwCreateWindowSurface failed: %d\n", res);
        return MC_ERR_VULKAN;
    }
    return MC_OK;
}

mc_error_t vk_select_physical_device(void)
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(g_render.instance, &count, NULL);
    if (count == 0) {
        fprintf(stderr, "No Vulkan-capable GPU found\n");
        return MC_ERR_VULKAN;
    }

    VkPhysicalDevice devices[16];
    if (count > 16) count = 16;
    vkEnumeratePhysicalDevices(g_render.instance, &count, devices);

    /* Pick first device with a graphics queue that also supports present */
    for (uint32_t i = 0; i < count; i++) {
        uint32_t qf_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qf_count, NULL);
        VkQueueFamilyProperties qf_props[64];
        if (qf_count > 64) qf_count = 64;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qf_count, qf_props);

        uint32_t gfx_family   = UINT32_MAX;
        uint32_t pres_family  = UINT32_MAX;

        for (uint32_t q = 0; q < qf_count; q++) {
            if (qf_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                gfx_family = q;

            VkBool32 present_support = VK_FALSE;
            if (g_render.surface) {
                vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], q,
                                                     g_render.surface,
                                                     &present_support);
            }
            if (present_support)
                pres_family = q;

            /* Prefer a family that does both */
            if (gfx_family != UINT32_MAX && pres_family != UINT32_MAX)
                break;
        }

        /* Accept device if we found a graphics family.
         * present_family may equal UINT32_MAX in headless mode (no surface). */
        if (gfx_family != UINT32_MAX) {
            g_render.physical_device = devices[i];
            g_render.graphics_family = gfx_family;
            g_render.present_family  = (pres_family != UINT32_MAX) ? pres_family : gfx_family;
            vkGetPhysicalDeviceMemoryProperties(devices[i], &g_render.mem_props);
            return MC_OK;
        }
    }

    fprintf(stderr, "No GPU with graphics queue found\n");
    return MC_ERR_VULKAN;
}

mc_error_t vk_create_device(void)
{
    float priority = 1.0f;

    VkDeviceQueueCreateInfo queue_cis[2];
    uint32_t queue_ci_count = 1;

    memset(&queue_cis[0], 0, sizeof(queue_cis[0]));
    queue_cis[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_cis[0].queueFamilyIndex = g_render.graphics_family;
    queue_cis[0].queueCount       = 1;
    queue_cis[0].pQueuePriorities = &priority;

    if (g_render.present_family != g_render.graphics_family) {
        memset(&queue_cis[1], 0, sizeof(queue_cis[1]));
        queue_cis[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_cis[1].queueFamilyIndex = g_render.present_family;
        queue_cis[1].queueCount       = 1;
        queue_cis[1].pQueuePriorities = &priority;
        queue_ci_count = 2;
    }

    const char *dev_exts[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
        "VK_KHR_portability_subset",
#endif
    };
    uint32_t dev_ext_count = sizeof(dev_exts) / sizeof(dev_exts[0]);

    VkDeviceCreateInfo ci;
    memset(&ci, 0, sizeof(ci));
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = queue_ci_count;
    ci.pQueueCreateInfos       = queue_cis;
    ci.enabledExtensionCount   = dev_ext_count;
    ci.ppEnabledExtensionNames = dev_exts;

    VkResult res = vkCreateDevice(g_render.physical_device, &ci, NULL, &g_render.device);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDevice failed: %d\n", res);
        return MC_ERR_VULKAN;
    }

    vkGetDeviceQueue(g_render.device, g_render.graphics_family, 0, &g_render.graphics_queue);
    vkGetDeviceQueue(g_render.device, g_render.present_family,  0, &g_render.present_queue);
    return MC_OK;
}

uint32_t vk_find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    for (uint32_t i = 0; i < g_render.mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1u << i)) &&
            (g_render.mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}
