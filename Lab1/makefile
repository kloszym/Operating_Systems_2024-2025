targets = countdown
CC = gcc
CFLAGS = -Wall -std=c17 -g

.PHONY: all
all: countdown

countdown: countdown.c
	$(CC) countdown.c $(CFLAGS) -o countdown

.PHONY: clean
clean:
	rm $(targets)