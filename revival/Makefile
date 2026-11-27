CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic -O2
.PHONY: test clean
test:
	$(CC) $(CFLAGS) -I. storage.c tests/storage_test.c -o storage_test
	./storage_test
	$(CC) $(CFLAGS) -I. present.c tests/present_test.c -o present_test
	./present_test
	$(CC) $(CFLAGS) -I. tests/timer_policy_test.c -o timer_policy_test
	./timer_policy_test
	$(CC) $(CFLAGS) -Ipcsx4all-probe tests/display_stage_test.c -o display_stage_test
	./display_stage_test
	$(CC) $(CFLAGS) -Ipcsx4all-probe tests/disc_preflight_test.c -o disc_preflight_test
	./disc_preflight_test
clean:
	rm -f storage_test present_test timer_policy_test display_stage_test disc_preflight_test
