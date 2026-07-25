CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 `pkg-config --cflags --libs sdl3`

build: main.c
	$(CC) $(CFLAGS) -g -o snake main.c

clean:
	rm ./snake

run: build
	./snake
