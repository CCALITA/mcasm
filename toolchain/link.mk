LD       ?= $(CC)
LDFLAGS   = $(LDFLAGS_PLATFORM)

ifdef DEBUG
  LDFLAGS += -fsanitize=address
endif
