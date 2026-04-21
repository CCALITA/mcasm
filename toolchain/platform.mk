UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Darwin)
  PLATFORM   := macos
  OBJ_EXT    := .o
  CFLAGS_PLATFORM := $(shell pkg-config --cflags glfw3 vulkan 2>/dev/null)
  LDFLAGS_PLATFORM := $(shell pkg-config --libs glfw3 vulkan 2>/dev/null) -framework Cocoa -framework IOKit -framework QuartzCore -L/opt/homebrew/lib -lMoltenVK

  ifeq ($(UNAME_M),x86_64)
    # Native x86_64: use NASM for ASM modules
    NASM_ENABLED := 1
    NASM_FMT     := macho64
    NASM_DEFS    := -DMACHO
    CC           := cc -arch x86_64
    LDFLAGS      := -arch x86_64
  else
    # ARM64 (Apple Silicon): skip NASM, use C fallbacks
    NASM_ENABLED := 0
    NASM_FMT     :=
    NASM_DEFS    :=
    CC           := cc
    LDFLAGS      :=
  endif

else ifeq ($(UNAME_S),Linux)
  PLATFORM   := linux
  NASM_FMT   := elf64
  NASM_DEFS  :=
  OBJ_EXT    := .o
  CC         := gcc
  NASM_ENABLED := 1
  CFLAGS_PLATFORM := $(shell pkg-config --cflags glfw3 vulkan 2>/dev/null)
  LDFLAGS_PLATFORM := $(shell pkg-config --libs glfw3 vulkan 2>/dev/null) -lm -lpthread
else
  $(error Unsupported platform: $(UNAME_S))
endif
