/*
 * defaults.c -- Register default Minecraft block types at init time.
 */

#include "mc_block.h"

/* Helper: solid, opaque block with no light emission */
static block_properties_t solid_block(float hardness, uint16_t tex_top,
                                      uint16_t tex_side, uint16_t tex_bottom)
{
    return (block_properties_t){
        .hardness       = hardness,
        .transparent    = 0,
        .solid          = 1,
        .light_emission = 0,
        .flammable      = 0,
        .texture_top    = tex_top,
        .texture_side   = tex_side,
        .texture_bottom = tex_bottom,
    };
}

/* Helper: non-solid, transparent block (attachables, redstone, etc.) */
static block_properties_t nonsolid_block(float hardness, uint8_t light,
                                         uint16_t tex)
{
    return (block_properties_t){
        .hardness       = hardness,
        .transparent    = 1,
        .solid          = 0,
        .light_emission = light,
        .flammable      = 0,
        .texture_top    = tex,
        .texture_side   = tex,
        .texture_bottom = tex,
    };
}

static mc_error_t register_defaults(void)
{
    mc_error_t err;

    /* AIR: transparent, not solid, no hardness */
    {
        block_properties_t p = {
            .hardness = 0.0f, .transparent = 1, .solid = 0,
            .light_emission = 0, .flammable = 0,
            .texture_top = 0, .texture_side = 0, .texture_bottom = 0,
        };
        err = mc_block_register(BLOCK_AIR, &p);
        if (err != MC_OK) return err;
    }

    /* STONE */
    {
        block_properties_t p = solid_block(1.5f, 0, 0, 0);
        err = mc_block_register(BLOCK_STONE, &p);
        if (err != MC_OK) return err;
    }

    /* GRASS */
    {
        block_properties_t p = solid_block(0.6f, 1, 2, 3);
        err = mc_block_register(BLOCK_GRASS, &p);
        if (err != MC_OK) return err;
    }

    /* DIRT */
    {
        block_properties_t p = solid_block(0.5f, 3, 3, 3);
        err = mc_block_register(BLOCK_DIRT, &p);
        if (err != MC_OK) return err;
    }

    /* COBBLESTONE */
    {
        block_properties_t p = solid_block(2.0f, 4, 4, 4);
        err = mc_block_register(BLOCK_COBBLESTONE, &p);
        if (err != MC_OK) return err;
    }

    /* OAK_PLANKS (flammable) */
    {
        block_properties_t p = solid_block(2.0f, 5, 5, 5);
        p.flammable = 1;
        err = mc_block_register(BLOCK_OAK_PLANKS, &p);
        if (err != MC_OK) return err;
    }

    /* BEDROCK (hardness = -1 => indestructible) */
    {
        block_properties_t p = solid_block(-1.0f, 6, 6, 6);
        err = mc_block_register(BLOCK_BEDROCK, &p);
        if (err != MC_OK) return err;
    }

    /* WATER: transparent, not solid */
    {
        block_properties_t p = {
            .hardness = 100.0f, .transparent = 1, .solid = 0,
            .light_emission = 0, .flammable = 0,
            .texture_top = 7, .texture_side = 7, .texture_bottom = 7,
        };
        err = mc_block_register(BLOCK_WATER, &p);
        if (err != MC_OK) return err;
    }

    /* LAVA: light=15, not transparent for rendering purposes */
    {
        block_properties_t p = {
            .hardness = 100.0f, .transparent = 0, .solid = 0,
            .light_emission = 15, .flammable = 0,
            .texture_top = 8, .texture_side = 8, .texture_bottom = 8,
        };
        err = mc_block_register(BLOCK_LAVA, &p);
        if (err != MC_OK) return err;
    }

    /* SAND */
    {
        block_properties_t p = solid_block(0.5f, 9, 9, 9);
        err = mc_block_register(BLOCK_SAND, &p);
        if (err != MC_OK) return err;
    }

    /* GRAVEL */
    {
        block_properties_t p = solid_block(0.6f, 10, 10, 10);
        err = mc_block_register(BLOCK_GRAVEL, &p);
        if (err != MC_OK) return err;
    }

    /* OAK_LOG (flammable) */
    {
        block_properties_t p = solid_block(2.0f, 11, 12, 11);
        p.flammable = 1;
        err = mc_block_register(BLOCK_OAK_LOG, &p);
        if (err != MC_OK) return err;
    }

    /* OAK_LEAVES: transparent, flammable */
    {
        block_properties_t p = {
            .hardness = 0.2f, .transparent = 1, .solid = 1,
            .light_emission = 0, .flammable = 1,
            .texture_top = 13, .texture_side = 13, .texture_bottom = 13,
        };
        err = mc_block_register(BLOCK_OAK_LEAVES, &p);
        if (err != MC_OK) return err;
    }

    /* GLASS: transparent */
    {
        block_properties_t p = {
            .hardness = 0.3f, .transparent = 1, .solid = 1,
            .light_emission = 0, .flammable = 0,
            .texture_top = 14, .texture_side = 14, .texture_bottom = 14,
        };
        err = mc_block_register(BLOCK_GLASS, &p);
        if (err != MC_OK) return err;
    }

    /* Ores */
    {
        block_properties_t p = solid_block(3.0f, 15, 15, 15);
        err = mc_block_register(BLOCK_IRON_ORE, &p);
        if (err != MC_OK) return err;
    }
    {
        block_properties_t p = solid_block(3.0f, 16, 16, 16);
        err = mc_block_register(BLOCK_COAL_ORE, &p);
        if (err != MC_OK) return err;
    }
    {
        block_properties_t p = solid_block(3.0f, 17, 17, 17);
        err = mc_block_register(BLOCK_GOLD_ORE, &p);
        if (err != MC_OK) return err;
    }
    {
        block_properties_t p = solid_block(3.0f, 18, 18, 18);
        err = mc_block_register(BLOCK_DIAMOND_ORE, &p);
        if (err != MC_OK) return err;
    }
    {
        block_properties_t p = solid_block(3.0f, 19, 19, 19);
        err = mc_block_register(BLOCK_REDSTONE_ORE, &p);
        if (err != MC_OK) return err;
    }

    /* OBSIDIAN */
    {
        block_properties_t p = solid_block(50.0f, 20, 20, 20);
        err = mc_block_register(BLOCK_OBSIDIAN, &p);
        if (err != MC_OK) return err;
    }

    /* TNT (flammable) */
    {
        block_properties_t p = solid_block(0.0f, 21, 22, 23);
        p.flammable = 1;
        err = mc_block_register(BLOCK_TNT, &p);
        if (err != MC_OK) return err;
    }

    /* CRAFTING_TABLE */
    {
        block_properties_t p = solid_block(2.5f, 24, 25, 5);
        p.flammable = 1;
        err = mc_block_register(BLOCK_CRAFTING_TABLE, &p);
        if (err != MC_OK) return err;
    }

    /* FURNACE */
    {
        block_properties_t p = solid_block(3.5f, 26, 27, 26);
        err = mc_block_register(BLOCK_FURNACE, &p);
        if (err != MC_OK) return err;
    }

    /* CHEST (flammable) */
    {
        block_properties_t p = solid_block(2.5f, 28, 29, 28);
        p.flammable = 1;
        err = mc_block_register(BLOCK_CHEST, &p);
        if (err != MC_OK) return err;
    }

    /* TORCH: light=14, not solid */
    {
        block_properties_t p = nonsolid_block(0.0f, 14, 30);
        err = mc_block_register(BLOCK_TORCH, &p);
        if (err != MC_OK) return err;
    }

    /* REDSTONE_WIRE: not solid */
    {
        block_properties_t p = nonsolid_block(0.0f, 0, 31);
        err = mc_block_register(BLOCK_REDSTONE_WIRE, &p);
        if (err != MC_OK) return err;
    }

    /* REDSTONE_TORCH: light=7, not solid */
    {
        block_properties_t p = nonsolid_block(0.0f, 7, 32);
        err = mc_block_register(BLOCK_REDSTONE_TORCH, &p);
        if (err != MC_OK) return err;
    }

    /* LEVER: not solid */
    {
        block_properties_t p = nonsolid_block(0.5f, 0, 33);
        err = mc_block_register(BLOCK_LEVER, &p);
        if (err != MC_OK) return err;
    }

    /* BUTTON: not solid */
    {
        block_properties_t p = nonsolid_block(0.5f, 0, 34);
        err = mc_block_register(BLOCK_BUTTON, &p);
        if (err != MC_OK) return err;
    }

    /* PISTON */
    {
        block_properties_t p = solid_block(1.5f, 35, 36, 37);
        err = mc_block_register(BLOCK_PISTON, &p);
        if (err != MC_OK) return err;
    }

    /* REPEATER: not solid */
    {
        block_properties_t p = nonsolid_block(0.0f, 0, 38);
        err = mc_block_register(BLOCK_REPEATER, &p);
        if (err != MC_OK) return err;
    }

    /* COMPARATOR: not solid */
    {
        block_properties_t p = nonsolid_block(0.0f, 0, 39);
        err = mc_block_register(BLOCK_COMPARATOR, &p);
        if (err != MC_OK) return err;
    }

    return MC_OK;
}

mc_error_t mc_block_init(void)
{
    return register_defaults();
}

void mc_block_shutdown(void)
{
    /* Nothing to free — the registry is a static BSS array in registry.asm. */
}
