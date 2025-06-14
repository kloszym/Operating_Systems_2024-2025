CC = gcc
CFLAGS = -Wall -std=c17
TARGET = hospital

.PHONY: all
all: $(TARGET)

hospital: hospital.c
	$(CC) $(CFLAGS) hospital.c -o hospital -lm -lpthread


run: $(TARGET)
	./$(TARGET) $(ARGS)

.PHONY: clean
clean:
	rm $(TARGET)