CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 `pkg-config --cflags --libs sdl3`

build: main.c
	$(CC) $(CFLAGS) -g -o snake main.c

build-wasm: main.c
	emcc --use-port=sdl3 -o web/snake.js main.c --embed-file ./numbers.bmp@/numbers.bmp

clean:
	rm ./snake

run: build
	./snake

debug: build
	gdb ./snake
