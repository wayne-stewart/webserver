.DEFAULT_GOAL := build

OUTROOT = build
CFLAGS = -Werror -Wall -Wextra -Wpedantic -Wfatal-errors

build: make_dir
	gcc src/main.c -o $(OUTROOT)/webserver $(CFLAGS) -O3

run: build
	$(OUTROOT)/webserver

debug: debug-build
	gdb $(OUTROOT)/webserver-debug

test: test-build
	$(OUTROOT)/test

debug-build: make_dir
	gcc src/main.c -o $(OUTROOT)/webserver-debug $(CFLAGS) -Og -g

test-build: make_dir
	gcc src/main_test.c -DTEST_BUILD -o $(OUTROOT)/test

make_dir:
	mkdir -p $(OUTROOT)


