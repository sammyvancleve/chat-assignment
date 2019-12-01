/*
chat-client.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    char *dest_hostname, *dest_port;
    struct addrinfo hints, *res;
    char buf[BUF_SIZE];
    int conn_fd;
    int n;
    int rc;

    dest_hostname = argv[1];
    dest_port = argv[2];

    conn_fd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((rc = getaddrinfo(dest_hostname, dest_port, &hints, &res)) != 0) {
        perror("getaddrinfo failed: ");
        exit(1);
    }
    if (connect(conn_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect: ");
        exit(2);
    }

    //while loop until user exits (ctrl-d)
    //maybe parse input from user before sending?
    //ex for name change...
    while((n = read(0, buf, BUF_SIZE)) > 0) {
        send(conn_fd, buf, n, 0);

        //todo separate thread for receiving info from server
        n = recv(conn_fd, buf, BUF_SIZE, 0);
        printf("received: ");
        puts(buf);
    }

    //send disconnect before exiting
    strcpy(buf, "/disconnect");
    send(conn_fd, buf, strlen("/disconnect", 0);

    close(conn_fd);
}
