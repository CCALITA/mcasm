TEST_SRC := $(wildcard test/*.c)
TEST_BIN := $(TEST_SRC:test/%.c=test/bin/%)

test: $(LIB) $(TEST_BIN)
	@for t in $(TEST_BIN); do echo "  TEST $$t"; ./$$t || exit 1; done
	@echo "  ALL TESTS PASSED ($(MODULE))"

test/bin/%: test/%.c $(LIB) | test/bin
	$(CC) $(CFLAGS) -o $@ $< -L. -l$(MODULE) $(TEST_LINK_DEPS) -lm

test/bin:
	@mkdir -p test/bin

.PHONY: test
