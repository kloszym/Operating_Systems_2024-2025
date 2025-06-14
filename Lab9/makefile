CC = gcc
CFLAGS = -Wall -std=c17 -g
TARGET = zad1

.PHONY: all
all: $(TARGET)

zad1: zad1.c
	$(CC) $(CFLAGS) zad1.c -o zad1 -lm -lpthread

run: $(TARGET)
	./$(TARGET) $(ARGS)

.PHONY: clean
clean:
	rm $(TARGET)