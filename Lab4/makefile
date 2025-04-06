CC = gcc
CFLAGS = -Wall -std=c17
TARGET = zad1 zad2

.PHONY: all
all: $(TARGET)

zad1: zad1.c
	$(CC) $(CFLAGS) zad1.c -o zad1

zad2: zad2.c
	$(CC) $(CFLAGS) zad2.c -o zad2

run: $(TARGET)
	./$(TARGET) $(ARGS)

.PHONY: clean
clean:
	rm $(TARGET)