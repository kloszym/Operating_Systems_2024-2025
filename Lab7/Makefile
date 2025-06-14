CC = gcc
CFLAGS = -Wall -std=c17 -lrt
TARGET = server client

.PHONY: all
all: $(TARGET)

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

.PHONY: clean
clean:
	rm -f $(TARGET)