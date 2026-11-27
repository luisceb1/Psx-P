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
	$(CC) $(CFLAGS) -I. tests/pcm_queue_test.c -o pcm_queue_test
	./pcm_queue_test
	$(CC) $(CFLAGS) -I. tests/audio_clock_test.c -o audio_clock_test
	./audio_clock_test
clean:
	rm -f storage_test present_test timer_policy_test pcm_queue_test audio_clock_test
