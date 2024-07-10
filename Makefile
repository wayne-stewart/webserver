.DEFAULT_GOAL := build

CFLAGS = -Werror -Wall -Wextra -Wpedantic -Wfatal-errors

build:
	gcc src/main.c -o webserver $(CFLAGS) -O3

run: build
	./webserver

debug-build:
	gcc src/main.c -o webserver-debug $(CFLAGS) -Og -g

debug: debug-build
	gdb webserver-debug


