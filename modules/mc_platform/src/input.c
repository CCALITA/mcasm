#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "mc_platform.h"
#include <string.h>

#define MAX_KEYS 512

/* Defined in window.c */
extern GLFWwindow* mc_platform_get_glfw_window(void);

/*
 * Two arrays: one for "held this frame", one for "pressed this frame".
 * A key is "pressed" only on the transition from released to held.
 */
static uint8_t g_key_held[MAX_KEYS];
static uint8_t g_key_pressed[MAX_KEYS];

static double g_mouse_x     = 0.0;
static double g_mouse_y     = 0.0;
static double g_mouse_dx    = 0.0;
static double g_mouse_dy    = 0.0;
static int    g_first_mouse = 1;

/* ---- GLFW callbacks ---- */

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;

    if (key < 0 || key >= MAX_KEYS) {
        return;
    }

    if (action == GLFW_PRESS) {
        g_key_held[key]    = 1;
        g_key_pressed[key] = 1;
    } else if (action == GLFW_RELEASE) {
        g_key_held[key]    = 0;
    }
    /* GLFW_REPEAT intentionally ignored for g_key_pressed. */
}

static void cursor_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;

    if (g_first_mouse) {
        g_mouse_x     = xpos;
        g_mouse_y     = ypos;
        g_first_mouse = 0;
    }

    g_mouse_dx += xpos - g_mouse_x;
    g_mouse_dy += ypos - g_mouse_y;
    g_mouse_x   = xpos;
    g_mouse_y   = ypos;
}

/* ---- public API (declared in mc_platform.h) ---- */

uint8_t mc_platform_key_pressed(int key) {
    if (key < 0 || key >= MAX_KEYS) {
        return 0;
    }
    uint8_t v = g_key_pressed[key];
    g_key_pressed[key] = 0; /* consumed on read */
    return v;
}

uint8_t mc_platform_key_held(int key) {
    if (key < 0 || key >= MAX_KEYS) {
        return 0;
    }
    return g_key_held[key];
}

void mc_platform_get_mouse_delta(float* dx, float* dy) {
    *dx = (float)g_mouse_dx;
    *dy = (float)g_mouse_dy;
    g_mouse_dx = 0.0;
    g_mouse_dy = 0.0;
}

void mc_platform_get_mouse_pos(float* x, float* y) {
    *x = (float)g_mouse_x;
    *y = (float)g_mouse_y;
}

void mc_platform_set_cursor_locked(uint8_t locked) {
    GLFWwindow* win = mc_platform_get_glfw_window();
    if (win == NULL) {
        return;
    }
    int mode = locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
    glfwSetInputMode(win, GLFW_CURSOR, mode);

    if (locked) {
        g_first_mouse = 1;
    }
}

void mc_platform_install_input_callbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_callback);

    memset(g_key_held, 0, sizeof(g_key_held));
    memset(g_key_pressed, 0, sizeof(g_key_pressed));
    g_mouse_dx    = 0.0;
    g_mouse_dy    = 0.0;
    g_first_mouse = 1;
}
