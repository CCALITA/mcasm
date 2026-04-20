MODULE := mc_redstone

include ../../toolchain/platform.mk
include ../../toolchain/nasm.mk
include ../../toolchain/cc.mk

# On non-x86_64 hosts (e.g. Apple Silicon) NASM produces x86_64 objects
# that cannot link with native ARM64 C objects.  Detect the mismatch and
# skip .asm files, relying on C fallbacks in src/power_decay_fallback.c.
HOST_ARCH := $(shell uname -m)
ifeq ($(HOST_ARCH),x86_64)
  ASM_SRC := $(wildcard src/*.asm)
else
  ASM_SRC :=
endif

C_SRC   := $(wildcard src/*.c)
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
