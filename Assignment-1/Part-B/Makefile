TESTING_CODE = $(wildcard tests/test-*.c)
TEST_FILES = $(patsubst %.c, %.out, $(TESTING_CODE))

obj-m += deque_lkm.o
all: deque_lkm.c
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

install:
	sudo insmod deque_lkm.ko

uninstall:
	sudo rmmod deque_lkm

tests:	$(TEST_FILES)

tests/test-%.out: tests/test-%.c
	gcc -o $@ $<

run_tests: $(TEST_FILES)
	@for test in $(TEST_FILES); do \
		echo "Running $$test"; \
		./$$test; \
	done

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm -f tests/test-*.out