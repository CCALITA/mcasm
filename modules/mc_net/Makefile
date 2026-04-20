MODULE := mc_net

include ../../toolchain/platform.mk
include ../../toolchain/nasm.mk
include ../../toolchain/cc.mk

ASM_SRC := $(wildcard src/*.asm)
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
