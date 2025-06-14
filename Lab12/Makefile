CC = gcc
CFLAGS = -Wall -std=c17
TARGET = server client

.PHONY: all
all: $(TARGET)

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client -lm -lpthread

run: $(TARGET)
	./$(TARGET) $(ARGS)

.PHONY: clean
clean:
	rm $(TARGET)