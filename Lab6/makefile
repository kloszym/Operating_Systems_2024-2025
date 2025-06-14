CC = gcc
CFLAGS = -Wall -std=c17
TARGET = zad1 zad2cat zad2sen

.PHONY: all
all: $(TARGET)

zad1: zad1.c
	$(CC) $(CFLAGS) zad1.c -o zad1 -lm

zad2sen: zad2sen.c zad2cat
	$(CC) $(CFLAGS) zad2sen.c -o zad2sen -lm

zad2cat: zad2cat.c
	$(CC) $(CFLAGS) zad2cat.c -o zad2cat -lm

run: $(TARGET)
	./$(TARGET) $(ARGS)

.PHONY: clean
clean:
	rm $(TARGET)