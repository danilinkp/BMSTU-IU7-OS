SRCS = $(wildcard *.c)
OUT = $(SRCS:.c=.out)

all: $(OUT)

%.out: %.c
	gcc -o $@ $< -lm

.PHONY: clean all
clean:
	rm -f *.out