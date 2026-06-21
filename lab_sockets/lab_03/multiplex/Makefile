CC := gcc

.PHONY: build
build: client server

%.exe: %.c
	gcc -o -std=c99 $@ $^ -Wall -Werror