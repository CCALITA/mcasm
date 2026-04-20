#include "mc_entity.h"
#include <string.h>
static uint32_t masks[MAX_ENTITIES]={0};
static transform_component_t transforms[MAX_ENTITIES]={0};
static physics_component_t physics[MAX_ENTITIES]={0};
static health_component_t healths[MAX_ENTITIES]={0};
static uint32_t next_id=1, count=0;
mc_error_t mc_entity_init(void) { next_id=1; count=0; memset(masks,0,sizeof(masks)); return MC_OK; }
void mc_entity_shutdown(void) {}
entity_id_t mc_entity_create(uint32_t mask) { if(next_id>=MAX_ENTITIES) return 0; uint32_t id=next_id++; masks[id]=mask; count++; return id; }
void mc_entity_destroy(entity_id_t id) { if(id<MAX_ENTITIES){masks[id]=0; count--;} }
uint8_t mc_entity_alive(entity_id_t id) { return id<MAX_ENTITIES && masks[id]!=0; }
uint32_t mc_entity_get_mask(entity_id_t id) { return id<MAX_ENTITIES ? masks[id] : 0; }
uint32_t mc_entity_count(void) { return count; }
transform_component_t* mc_entity_get_transform(entity_id_t id) { return id<MAX_ENTITIES ? &transforms[id] : NULL; }
physics_component_t* mc_entity_get_physics(entity_id_t id) { return id<MAX_ENTITIES ? &physics[id] : NULL; }
health_component_t* mc_entity_get_health(entity_id_t id) { return id<MAX_ENTITIES ? &healths[id] : NULL; }
void mc_entity_set_position(entity_id_t id, vec3_t pos) { if(id<MAX_ENTITIES) transforms[id].position=pos; }
void mc_entity_set_velocity(entity_id_t id, vec3_t vel) { if(id<MAX_ENTITIES) transforms[id].velocity=vel; }
void mc_entity_foreach(uint32_t req, mc_entity_system_fn fn, void* ud) { for(uint32_t i=1;i<next_id;i++) if((masks[i]&req)==req) fn(i,ud); }
