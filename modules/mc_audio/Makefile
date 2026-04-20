MODULE := mc_audio

include ../../toolchain/platform.mk
include ../../toolchain/nasm.mk
include ../../toolchain/cc.mk

# ------- OpenAL detection -------
ifeq ($(PLATFORM),macos)
  OPENAL_CFLAGS  :=
  OPENAL_LDFLAGS := -framework OpenAL
else
  OPENAL_AVAILABLE := $(shell pkg-config --exists openal-soft 2>/dev/null && echo 1 || echo 0)
  ifeq ($(OPENAL_AVAILABLE),1)
    OPENAL_CFLAGS  := $(shell pkg-config --cflags openal-soft) -DHAS_OPENAL=1
    OPENAL_LDFLAGS := $(shell pkg-config --libs openal-soft)
  else
    OPENAL_CFLAGS  := -DHAS_OPENAL=0
    OPENAL_LDFLAGS :=
  endif
endif

CFLAGS += $(OPENAL_CFLAGS)

ASM_SRC := $(wildcard src/*.asm)
C_SRC   := $(wildcard src/*.c)
ASM_OBJ := $(ASM_SRC:src/%.asm=obj/%.o)
C_OBJ   := $(C_SRC:src/%.c=obj/%.o)
ALL_OBJ := $(ASM_OBJ) $(C_OBJ)
LIB     := lib$(MODULE).a

TEST_LINK_DEPS := $(OPENAL_LDFLAGS)

.PHONY: all clean

all: $(LIB)

$(LIB): $(ALL_OBJ)
	ar rcs $@ $^

obj:
	@mkdir -p obj

clean:
	rm -rf obj $(LIB) test/bin

include ../../toolchain/test.mk
