#include "mc_input.h"
mc_error_t mc_input_init(void) { return MC_OK; }
void mc_input_shutdown(void) {}
void mc_input_update(void) {}
uint8_t mc_input_action_pressed(uint8_t a) { (void)a; return 0; }
uint8_t mc_input_action_held(uint8_t a) { (void)a; return 0; }
uint8_t mc_input_action_released(uint8_t a) { (void)a; return 0; }
void mc_input_get_look_delta(float* y, float* p) { *y=0; *p=0; }
float mc_input_get_move_forward(void) { return 0; }
float mc_input_get_move_strafe(void) { return 0; }
mc_error_t mc_input_bind_key(uint8_t a, int k) { (void)a;(void)k; return MC_OK; }
int mc_input_get_binding(uint8_t a) { (void)a; return 0; }
