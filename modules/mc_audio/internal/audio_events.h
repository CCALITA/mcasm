#ifndef MC_AUDIO_EVENTS_H
#define MC_AUDIO_EVENTS_H

#include "mc_types.h"
#include <stdint.h>

/* Sound event identifiers */
#define SOUND_FOOTSTEP      0
#define SOUND_BLOCK_BREAK   1
#define SOUND_BLOCK_PLACE   2
#define SOUND_HURT          3
#define SOUND_AMBIENT_CAVE  4
#define SOUND_WATER_AMBIENT 5
#define SOUND_MUSIC         6
#define SOUND_EVENT_COUNT   7

/*
 * Initialise built-in sound events.
 * Must be called after mc_audio_init().
 * Generates placeholder PCM tones and registers them with the audio engine.
 */
void mc_audio_events_init(void);

/*
 * Play a registered sound event at the given 3D position.
 * event must be < SOUND_EVENT_COUNT. Invalid IDs are silently ignored.
 */
void mc_audio_play_event(uint8_t event, vec3_t position);

#endif /* MC_AUDIO_EVENTS_H */
