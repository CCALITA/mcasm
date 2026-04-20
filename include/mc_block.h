#ifndef MC_BLOCK_H
#define MC_BLOCK_H

#include "mc_types.h"
#include "mc_error.h"

#define BLOCK_AIR              0
#define BLOCK_STONE            1
#define BLOCK_GRASS            2
#define BLOCK_DIRT             3
#define BLOCK_COBBLESTONE      4
#define BLOCK_OAK_PLANKS       5
#define BLOCK_BEDROCK          6
#define BLOCK_WATER            7
#define BLOCK_LAVA             8
#define BLOCK_SAND             9
#define BLOCK_GRAVEL           10
#define BLOCK_OAK_LOG          11
#define BLOCK_OAK_LEAVES       12
#define BLOCK_GLASS            13
#define BLOCK_IRON_ORE         14
#define BLOCK_COAL_ORE         15
#define BLOCK_GOLD_ORE         16
#define BLOCK_DIAMOND_ORE      17
#define BLOCK_REDSTONE_ORE     18
#define BLOCK_OBSIDIAN         19
#define BLOCK_TNT              20
#define BLOCK_CRAFTING_TABLE   21
#define BLOCK_FURNACE          22
#define BLOCK_CHEST            23
#define BLOCK_TORCH            24
#define BLOCK_REDSTONE_WIRE    25
#define BLOCK_REDSTONE_TORCH   26
#define BLOCK_LEVER            27
#define BLOCK_BUTTON           28
#define BLOCK_PISTON           29
#define BLOCK_REPEATER         30
#define BLOCK_COMPARATOR       31

#define BLOCK_TYPE_COUNT       256

typedef struct {
    float    hardness;
    uint8_t  transparent;
    uint8_t  solid;
    uint8_t  light_emission;
    uint8_t  flammable;
    uint16_t texture_top;
    uint16_t texture_side;
    uint16_t texture_bottom;
} block_properties_t;

mc_error_t              mc_block_init(void);
void                    mc_block_shutdown(void);
mc_error_t              mc_block_register(block_id_t id, const block_properties_t* props);
const block_properties_t* mc_block_get_properties(block_id_t id);
uint8_t                 mc_block_is_solid(block_id_t id);
uint8_t                 mc_block_is_transparent(block_id_t id);
uint8_t                 mc_block_get_light(block_id_t id);

#endif /* MC_BLOCK_H */
