include toolchain/platform.mk
include toolchain/link.mk

LDFLAGS := $(LDFLAGS_PLATFORM)

# ------- OpenAL link flags -------
ifeq ($(PLATFORM),macos)
  OPENAL_LDFLAGS := -framework OpenAL
else
  ifeq ($(shell pkg-config --exists openal-soft 2>/dev/null && echo 1),1)
    OPENAL_LDFLAGS := $(shell pkg-config --libs openal-soft)
  else
    OPENAL_LDFLAGS :=
  endif
endif

MODULES := mc_memory mc_math mc_platform mc_block mc_world mc_worldgen \
           mc_entity mc_mob_ai mc_physics mc_render mc_audio mc_ui \
           mc_input mc_net mc_save mc_crafting mc_redstone mc_particle \
           mc_command mc_main

MODULE_LIBS := $(foreach m,$(MODULES),modules/$(m)/lib$(m).a)

GLSLC := glslangValidator
SHADER_SRC := $(wildcard modules/mc_render/shaders/*.vert.glsl) \
              $(wildcard modules/mc_render/shaders/*.frag.glsl) \
              $(wildcard modules/mc_render/shaders/*.comp.glsl)
SHADER_SPV := $(SHADER_SRC:.glsl=.spv)

.PHONY: all clean test lint abi-check modules

all: mcasm $(SHADER_SPV)

mcasm: $(MODULE_LIBS)
	$(CC) -o $@ $(foreach lib,$^,-Wl,-force_load,$(lib)) $(LDFLAGS) $(OPENAL_LDFLAGS)

define MODULE_TEMPLATE
modules/$(1)/lib$(1).a: FORCE
	@$$(MAKE) -C modules/$(1) NASM_FMT=$(NASM_FMT) NASM_DEFS="$(NASM_DEFS)" \
	  CFLAGS_PLATFORM="$(CFLAGS_PLATFORM)" LDFLAGS_PLATFORM="$(LDFLAGS_PLATFORM)"
endef

$(foreach m,$(MODULES),$(eval $(call MODULE_TEMPLATE,$(m))))

FORCE:

%.vert.spv: %.vert.glsl
	$(GLSLC) -V -S vert -o $@ $<

%.frag.spv: %.frag.glsl
	$(GLSLC) -V -S frag -o $@ $<

%.comp.spv: %.comp.glsl
	$(GLSLC) -V -S comp -o $@ $<

test:
	@for m in $(MODULES); do \
		echo "=== Testing $$m ==="; \
		$(MAKE) -C modules/$$m test NASM_FMT=$(NASM_FMT) NASM_DEFS="$(NASM_DEFS)" \
		  CFLAGS_PLATFORM="$(CFLAGS_PLATFORM)" LDFLAGS_PLATFORM="$(LDFLAGS_PLATFORM)" \
		  || exit 1; \
	done
	@echo "=== ALL MODULE TESTS PASSED ==="

lint:
	python3 toolchain/ci/lint-asm.py $$(find modules -name '*.asm')

abi-check: $(MODULE_LIBS)
	python3 toolchain/ci/abi-check.py include/ $(MODULE_LIBS)

clean:
	@for m in $(MODULES); do $(MAKE) -C modules/$$m clean; done
	rm -f mcasm
	rm -f modules/mc_render/shaders/*.spv
