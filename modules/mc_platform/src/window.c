#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "mc_platform.h"

/* Defined in input.c */
extern void mc_platform_install_input_callbacks(GLFWwindow* window);

static GLFWwindow* g_window = NULL;

mc_error_t mc_platform_init(uint32_t width, uint32_t height, const char* title) {
    if (g_window != NULL) {
        return MC_ERR_ALREADY_EXISTS;
    }

    if (!glfwInit()) {
        return MC_ERR_PLATFORM;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    g_window = glfwCreateWindow((int)width, (int)height, title, NULL, NULL);
    if (g_window == NULL) {
        glfwTerminate();
        return MC_ERR_PLATFORM;
    }

    mc_platform_install_input_callbacks(g_window);

    return MC_OK;
}

void mc_platform_shutdown(void) {
    if (g_window != NULL) {
        glfwDestroyWindow(g_window);
        g_window = NULL;
    }
    glfwTerminate();
}

void mc_platform_poll_events(void) {
    glfwPollEvents();
}

uint8_t mc_platform_should_close(void) {
    if (g_window == NULL) {
        return 1;
    }
    return glfwWindowShouldClose(g_window) ? 1 : 0;
}

void mc_platform_swap_buffers(void) {
    /* No-op: Vulkan handles presentation, not GLFW swap buffers. */
}

void* mc_platform_get_window_handle(void) {
    return (void*)g_window;
}

void mc_platform_get_framebuffer_size(uint32_t* width, uint32_t* height) {
    if (g_window == NULL) {
        *width = 0;
        *height = 0;
        return;
    }
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(g_window, &w, &h);
    *width = (uint32_t)w;
    *height = (uint32_t)h;
}

const char** mc_platform_get_vulkan_extensions(uint32_t* count) {
    return glfwGetRequiredInstanceExtensions(count);
}

GLFWwindow* mc_platform_get_glfw_window(void) {
    return g_window;
}
