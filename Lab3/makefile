CC = gcc
CFLAGS = -Wall -std=c17
TARGET = flipper

.PHONY: all
all: $(TARGET)

flipper: flipper.c
	$(CC) $(CFLAGS) flipper.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS)

.PHONY: clean
clean:
	rm $(TARGET)