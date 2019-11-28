#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG 10
#define BUF_SIZE 4096

int main(int arvc, char *argv[]) {
    char *listen_port;
    int listen_fd, conn_fd;
    struct addrinfo hints, *res;
    struct sockaddr_in remote_sa;
    uint16_t remote_port;
    socklen_t addrlen;
    char *remote_ip;
    char buf[BUF_SIZE];
    int rc;
    int byte_received;

    listen_port = argv[1];

    listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if((rc = getaddrinfo(NULL, listen_port, &hints, &res)) != 0) {
        perror("getaddrinfo failed: ");
        exit(1);
    }

    bind(listen_fd, res->ai_addr, res->ai_addrlen);

    listen(listen_fd, BACKLOG);

    while(1) {
        addrlen = sizeof(remote_sa);
        conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen);

        remote_ip = inet_ntoa(remote_sa.sin_addr);
        remote_port = ntohs(remote_sa.sin_port);
        printf("new connection from %s:%d\n", remote_ip, remote_port);

        while((bytes_received = recv(conn_fd, buf, BUF_SIZE, 0)) > 0) {
            printf(".");
            fflush(stdout);

            send(conn_fd, buf, bytes_received, 0);
        }
        printf("\n");

        close(conn_fd);
    }

}
