CC = gcc
CFLAGS = -Wall -std=c17 -g
TARGET = printer user

.PHONY: all
all: $(TARGET)

printer: printer.c lab8.o
	$(CC) $(CFLAGS) printer.c lab8.o -o printer -lrt

user: user.c lab8.o
	$(CC) $(CFLAGS) user.c lab8.o -o user -lrt

lab8.o: lab8.h lab8.c
	$(CC) $(CFLAGS) -c lab8.c -o lab8.o

.PHONY: clean
clean:
	rm -f $(TARGET) lab8.o

.PHONY: forceclean
forceclean:
	sudo rm -f /dev/shm/sem.empty
	sudo rm -f /dev/shm/sem.filled
	sudo rm -f /dev/shm/sem.mutex
	sudo rm -f /dev/shm/shm
