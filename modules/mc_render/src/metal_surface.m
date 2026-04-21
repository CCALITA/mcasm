#ifdef __APPLE__
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

VkResult mc_create_metal_surface(VkInstance instance, GLFWwindow *window, VkSurfaceKHR *surface)
{
    NSWindow *ns_window = glfwGetCocoaWindow(window);
    if (!ns_window) return VK_ERROR_INITIALIZATION_FAILED;

    NSView *view = [ns_window contentView];
    [view setWantsLayer:YES];

    CAMetalLayer *metal_layer = [CAMetalLayer layer];
    [view setLayer:metal_layer];

    VkMetalSurfaceCreateInfoEXT ci = {
        .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .pLayer = metal_layer,
    };

    PFN_vkCreateMetalSurfaceEXT fn =
        (PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(instance, "vkCreateMetalSurfaceEXT");
    if (!fn) return VK_ERROR_EXTENSION_NOT_PRESENT;

    return fn(instance, &ci, NULL, surface);
}
#endif
