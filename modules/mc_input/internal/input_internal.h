#ifndef MC_INPUT_INTERNAL_H
#define MC_INPUT_INTERNAL_H

#include "mc_input.h"

/*
 * GLFW key codes used for default bindings.
 * Defined here so the input module doesn't #include <GLFW/glfw3.h>.
 */
#define KEY_SPACE           32
#define KEY_0               48
#define KEY_1               49
#define KEY_2               50
#define KEY_3               51
#define KEY_4               52
#define KEY_5               53
#define KEY_6               54
#define KEY_7               55
#define KEY_8               56
#define KEY_9               57
#define KEY_A               65
#define KEY_D               68
#define KEY_E               69
#define KEY_S               83
#define KEY_T               84
#define KEY_W               87
#define KEY_ESCAPE          256
#define KEY_F3              292
#define KEY_LEFT_SHIFT      340
#define KEY_LEFT_CONTROL    341

/* Maximum valid GLFW key code we need to track */
#define KEY_MAX             400

/* Per-action state flags (bitmask) */
#define STATE_HELD          0x01
#define STATE_PREV          0x02

/* Shared module state */
typedef struct {
    int     bindings[ACTION_COUNT];       /* action -> key code           */
    uint8_t action_state[ACTION_COUNT];   /* current + previous frame     */
    float   mouse_dx;                     /* accumulated mouse delta X    */
    float   mouse_dy;                     /* accumulated mouse delta Y    */
    float   sensitivity;                  /* mouse sensitivity multiplier */
} input_state_t;

/* Single global instance (defined in input.c) */
extern input_state_t g_input;

/* Called by input.c during init */
void mc_input_set_default_bindings(void);

#endif /* MC_INPUT_INTERNAL_H */
