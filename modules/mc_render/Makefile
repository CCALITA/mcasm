MODULE := mc_render

include ../../toolchain/platform.mk
include ../../toolchain/nasm.mk
include ../../toolchain/cc.mk

CFLAGS += -I internal/

ASM_SRC := $(wildcard src/*.asm)
C_SRC   := $(wildcard src/*.c)
M_SRC   := $(wildcard src/*.m)
ASM_OBJ := $(ASM_SRC:src/%.asm=obj/%.o)
C_OBJ   := $(C_SRC:src/%.c=obj/%.o)
M_OBJ   := $(M_SRC:src/%.m=obj/%.o)
ALL_OBJ := $(ASM_OBJ) $(C_OBJ) $(M_OBJ)
LIB     := lib$(MODULE).a

TEST_LINK_DEPS := $(LDFLAGS_PLATFORM)

.PHONY: all clean

all: $(LIB)

$(LIB): $(ALL_OBJ)
	ar rcs $@ $^

obj:
	@mkdir -p obj

clean:
	rm -rf obj $(LIB) test/bin

obj/%.o: src/%.m | obj
	$(CC) $(CFLAGS) -c -o $@ $<

include ../../toolchain/test.mk
