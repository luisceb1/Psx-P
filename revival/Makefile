CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic -O2
.PHONY: test clean
test:
	$(CC) $(CFLAGS) -I. storage.c tests/storage_test.c -o storage_test
	./storage_test
	$(CC) $(CFLAGS) -I. present.c tests/present_test.c -o present_test
	./present_test
clean:
	rm -f storage_test present_test
