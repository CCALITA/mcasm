UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  PLATFORM   := macos
  NASM_FMT   := macho64
  NASM_DEFS  := -DMACHO
  OBJ_EXT    := .o
  CC         := gcc
  CFLAGS_PLATFORM := $(shell pkg-config --cflags glfw3 vulkan 2>/dev/null)
  LDFLAGS_PLATFORM := $(shell pkg-config --libs glfw3 vulkan 2>/dev/null) -framework Cocoa -framework IOKit -framework QuartzCore
else ifeq ($(UNAME_S),Linux)
  PLATFORM   := linux
  NASM_FMT   := elf64
  NASM_DEFS  :=
  OBJ_EXT    := .o
  CC         := gcc
  CFLAGS_PLATFORM := $(shell pkg-config --cflags glfw3 vulkan 2>/dev/null)
  LDFLAGS_PLATFORM := $(shell pkg-config --libs glfw3 vulkan 2>/dev/null) -lm -lpthread
else
  $(error Unsupported platform: $(UNAME_S))
endif
