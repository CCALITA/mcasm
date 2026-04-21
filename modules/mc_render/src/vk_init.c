#include "render_internal.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

    uint32_t glfw_ext_count = 0;
    const char **glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    /* Fallback: if GLFW returns 0 extensions (Homebrew macOS), hardcode them */
    const char *fallback_exts[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef __APPLE__
        "VK_EXT_metal_surface",
#else
        "VK_KHR_xcb_surface",
#endif
    };
    uint32_t fallback_count = sizeof(fallback_exts) / sizeof(fallback_exts[0]);

    if (glfw_ext_count == 0) {
        glfw_exts = fallback_exts;
        glfw_ext_count = fallback_count;
    }

    uint32_t ext_count = glfw_ext_count;
    const char **extensions = malloc((ext_count + 2) * sizeof(char*));
    for (uint32_t i = 0; i < glfw_ext_count; i++)
        extensions[i] = glfw_exts[i];
#ifdef __APPLE__
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
    free(extensions);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed: %d\n", res);
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

    /* Pick first device with a graphics queue */
    for (uint32_t i = 0; i < count; i++) {
        uint32_t qf_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qf_count, NULL);
        VkQueueFamilyProperties qf_props[64];
        if (qf_count > 64) qf_count = 64;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qf_count, qf_props);

        for (uint32_t q = 0; q < qf_count; q++) {
            if (qf_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                g_render.physical_device = devices[i];
                g_render.graphics_family = q;
                vkGetPhysicalDeviceMemoryProperties(devices[i], &g_render.mem_props);
                return MC_OK;
            }
        }
    }

    fprintf(stderr, "No GPU with graphics queue found\n");
    return MC_ERR_VULKAN;
}

mc_error_t vk_create_device(void)
{
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_ci;
    memset(&queue_ci, 0, sizeof(queue_ci));
    queue_ci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci.queueFamilyIndex = g_render.graphics_family;
    queue_ci.queueCount       = 1;
    queue_ci.pQueuePriorities = &priority;

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
    ci.queueCreateInfoCount    = 1;
    ci.pQueueCreateInfos       = &queue_ci;
    ci.enabledExtensionCount   = dev_ext_count;
    ci.ppEnabledExtensionNames = dev_exts;

    VkResult res = vkCreateDevice(g_render.physical_device, &ci, NULL, &g_render.device);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateDevice failed: %d\n", res);
        return MC_ERR_VULKAN;
    }

    vkGetDeviceQueue(g_render.device, g_render.graphics_family, 0, &g_render.graphics_queue);
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
