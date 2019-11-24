CFLAGS=-Wall -pedantic -g

chat-server: chat-server.c
	gcc $(CFLAGS) -o chat-server chat-server.c

chat-client: chat-client.c
	gcc $(CFLAGS) -o chat-client chat-client.c

.PHONY: clean
clean:
	rm -f chat-client
	rm -f chat-server
