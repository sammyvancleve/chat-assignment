CFLAGS=-Wall -pedantic -g
LDFLAGS=-lpthread

all: chat-server chat-client

chat-server : chat-server.c
	gcc $(CFLAGS) $(LDFLAGS) -o chat-server chat-server.c

chat-clint: chat-client.c
	gcc $(CFLAGS) $(LDFLAGS) -o chat-client chat-client.c

.PHONY: clean
clean:
	rm -f chat-client
	rm -f chat-server
