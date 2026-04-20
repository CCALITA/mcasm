CC       ?= gcc
CFLAGS    = -std=c17 -Wall -Wextra -Werror -Wno-missing-field-initializers -O2 -g -I ../../include $(CFLAGS_PLATFORM)

ifdef DEBUG
  CFLAGS = -std=c17 -Wall -Wextra -Werror -Wno-missing-field-initializers -O0 -g3 -fsanitize=address -I ../../include $(CFLAGS_PLATFORM)
endif

obj/%.o: src/%.c | obj
	$(CC) $(CFLAGS) -I internal/ -c -o $@ $<
