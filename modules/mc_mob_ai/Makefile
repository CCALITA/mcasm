MODULE := mc_mob_ai

include ../../toolchain/platform.mk
include ../../toolchain/nasm.mk
include ../../toolchain/cc.mk

# On x86_64, build pathfind.asm; on ARM, use the C fallback
ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
  ASM_SRC := $(wildcard src/*.asm)
  C_SRC   := $(filter-out src/pathfind_c.c,$(wildcard src/*.c))
else
  ASM_SRC :=
  C_SRC   := $(filter-out src/stub.c,$(wildcard src/*.c))
endif

ASM_OBJ := $(ASM_SRC:src/%.asm=obj/%.o)
C_OBJ   := $(C_SRC:src/%.c=obj/%.o)
ALL_OBJ := $(ASM_OBJ) $(C_OBJ)
LIB     := lib$(MODULE).a

TEST_LINK_DEPS :=

.PHONY: all clean

all: $(LIB)

$(LIB): $(ALL_OBJ)
	ar rcs $@ $^

obj:
	@mkdir -p obj

clean:
	rm -rf obj $(LIB) test/bin

include ../../toolchain/test.mk
