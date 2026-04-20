# mcasm

A complete Minecraft clone built with x86-64 Assembly (NASM) and Vulkan.

## Architecture

The engine uses a hybrid approach: performance-critical inner loops (meshing, physics, math, noise generation) are hand-written x86-64 NASM assembly with SSE/AVX2 optimizations, while Vulkan API interaction and platform abstraction use thin C shims.

19 independent modules communicate through stable ABI contracts defined in `include/*.h`, enabling parallel development by large teams.

## Building

### Prerequisites

- NASM 2.16+ (`brew install nasm` or `apt install nasm`)
- GCC 12+ or Clang 15+
- Vulkan SDK (headers + loader)
- GLFW 3.3+ (`brew install glfw`)
- MoltenVK (macOS only: `brew install molten-vk`)
- glslangValidator (`brew install glslang`)

### Build

```bash
make        # Build all modules and link
make test   # Run all tests
make clean  # Clean artifacts
```

## Module Structure

| Module | Purpose |
|--------|---------|
| mc_memory | Arena and pool allocators |
| mc_math | SSE/AVX2 vector, matrix, noise |
| mc_platform | Window, input, timer (GLFW) |
| mc_render | Vulkan pipeline, chunk meshing |
| mc_world | Chunk storage, block access |
| mc_worldgen | Terrain generation, biomes |
| mc_block | Block type registry |
| mc_entity | ECS with SoA layout |
| mc_mob_ai | Pathfinding, behavior trees |
| mc_physics | Collision, gravity, raycasting |
| mc_audio | OpenAL 3D audio |
| mc_ui | HUD, menus, text rendering |
| mc_input | Key bindings, action mapping |
| mc_net | Client-server protocol |
| mc_save | NBT format, world persistence |
| mc_crafting | Recipe system |
| mc_redstone | Signal propagation |
| mc_particle | GPU particle system |
| mc_command | Chat commands |
| mc_main | Game loop orchestrator |

## License

MIT
