# mcasm — Minecraft Clone in Assembly + Vulkan

## Build

```bash
brew install nasm glfw vulkan-headers vulkan-loader molten-vk glslang
make          # build all modules + link
make test     # run all module tests
make clean    # clean build artifacts
```

## Architecture

- **Hybrid ASM+C**: x86-64 NASM for hot paths, C for Vulkan API / platform / tests
- **19 modules** in `modules/mc_*/`, each with own Makefile, src/, test/
- **Interface contracts**: `include/*.h` — public C headers defining module APIs
- **Shared ASM types**: `shared/*.inc` — NASM struct defs mirroring C headers
- **Cross-platform**: macOS (Mach-O 64 + MoltenVK) + Linux (ELF64 + native Vulkan)

## Conventions

- All module boundaries use **System V AMD64 ABI**
- ASM functions prefixed with module name: `mc_math_vec3_add`, `mc_world_get_block`
- Cross-platform symbol naming via `shared/platform.inc` DECL() macro
- Tests written in C, calling ASM via standard ABI
- No cross-module pointer sharing — copy semantics or handle/ID systems only
- `modules/*/internal/` is private to that module — never include from outside

## Module Build

Each module: `make -C modules/mc_<name>` produces `libmc_<name>.a`
Tests: `make -C modules/mc_<name> test`
