#ifndef MC_INPUT_H
#define MC_INPUT_H

#include "mc_types.h"
#include "mc_error.h"

#define ACTION_FORWARD     0
#define ACTION_BACKWARD    1
#define ACTION_LEFT        2
#define ACTION_RIGHT       3
#define ACTION_JUMP        4
#define ACTION_SNEAK       5
#define ACTION_SPRINT      6
#define ACTION_DIG         7
#define ACTION_PLACE       8
#define ACTION_USE         9
#define ACTION_INVENTORY   10
#define ACTION_CHAT        11
#define ACTION_PAUSE       12
#define ACTION_DEBUG       13
#define ACTION_HOTBAR_1    14
#define ACTION_HOTBAR_9    22
#define ACTION_COUNT       23

mc_error_t mc_input_init(void);
void       mc_input_shutdown(void);
void       mc_input_update(void);

uint8_t    mc_input_action_pressed(uint8_t action);
uint8_t    mc_input_action_held(uint8_t action);
uint8_t    mc_input_action_released(uint8_t action);

void       mc_input_get_look_delta(float* yaw, float* pitch);
float      mc_input_get_move_forward(void);
float      mc_input_get_move_strafe(void);

mc_error_t mc_input_bind_key(uint8_t action, int key);
int        mc_input_get_binding(uint8_t action);

#endif /* MC_INPUT_H */
