CC = gcc
CFLAGS = -Wall -std=c17
TARGET = zad1 catcher sender

.PHONY: all
all: $(TARGET)

zad1: zad1.c
	$(CC) $(CFLAGS) zad1.c -o zad1

catcher: catcher.c
	$(CC) $(CFLAGS) catcher.c -o catcher

sender: sender.c
	$(CC) $(CFLAGS) sender.c -o sender

run: $(TARGET)
	./$(TARGET) $(ARGS)

.PHONY: clean
clean:
	rm $(TARGET)