NASM     ?= nasm
NASMFLAGS = -f $(NASM_FMT) $(NASM_DEFS) -g -w+all -w-reloc-rel-dword -I ../../shared/ -I internal/

ifdef DEBUG
  NASMFLAGS += -DDEBUG
endif

ifeq ($(NASM_ENABLED),1)
obj/%.o: src/%.asm | obj
	$(NASM) $(NASMFLAGS) -o $@ $<
endif
