CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic -O2
.PHONY: test clean inspect
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
	@set -e; d=$$(mktemp -d); test -n "$$d"; trap 'rm -f "$$d/GAME.CUE" "$$d/Fixture.bin"; rmdir "$$d"' EXIT; ./disc_preflight_test "$$d"; $(CC) $(CFLAGS) -Ipcsx4all-probe pcsx4all-probe/inspect_cli.c -o disc_inspect; ./disc_inspect "$$d"
	$(CC) $(CFLAGS) -Ipcsx4all-probe tests/audio_queue_test.c -o audio_queue_test
	./audio_queue_test
	$(CC) $(CFLAGS) -Ipcsx4all-probe tests/disc_session_test.c -o disc_session_test
	./disc_session_test
	$(CC) $(CFLAGS) -Ipcsx4all-probe tests/core_session_test.c -o core_session_test
	./core_session_test
inspect:
	$(CC) $(CFLAGS) -Ipcsx4all-probe pcsx4all-probe/inspect_cli.c -o disc_inspect
clean:
	rm -f storage_test present_test timer_policy_test display_stage_test disc_preflight_test disc_inspect audio_queue_test disc_session_test core_session_test
