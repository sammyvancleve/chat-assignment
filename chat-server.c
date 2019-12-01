/*
   chat-server.c
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define BACKLOG 10
#define BUF_SIZE 4096
#define NUM_CLIENTS 6

void *connectionhandler(void *threadid);
void disconnectstring(char *s, char *user);

struct threadconn {
    socklen_t addrlen;
    struct sockaddr_in remote_sa;
    int conn_fd;
    int clientnum;
};

int main(int arvc, char *argv[]) {
    char *listen_port;
    int listen_fd;
    struct addrinfo hints, *res;
    //struct sockaddr_in remote_sa;
    //socklen_t addrlen;
    //char buf[BUF_SIZE];
    int rc;

    //userinfo strings
    //char *client_ip;
    //uint16_t client_port;

    pthread_t client_threads[NUM_CLIENTS];
    struct threadconn client_conns[NUM_CLIENTS];

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

    //start listening
    listen(listen_fd, BACKLOG);

    /* infinite loop accepting new connections and handling them */
    while(1) {
        //how to send something to all threads thats been received
        //from one...

        //accept new connection (will block until one appears)
        for (int i = 0; i < NUM_CLIENTS; i++) {
            client_conns[i].addrlen = sizeof(client_conns[i].remote_sa);
            client_conns[i].clientnum = i;
            client_conns[i].conn_fd = accept(listen_fd, (struct sockaddr *)
            &client_conns[i].remote_sa, &client_conns[i].addrlen);
            if (pthread_create(&client_threads[i], NULL, connectionhandler,
            &client_conns[i]) > 0) {
                printf("spun out thread %d\n", i);
            }
        }
        for (int i = 0; i < NUM_CLIENTS; i++) {
            pthread_join(client_threads[i], NULL);
        }
    }
}

void *connectionhandler(void *threadid) {
    struct threadconn conn = *((struct threadconn *) threadid);
    char *client_ip;
    uint16_t client_port;
    int bytes_received;
    char buf[BUF_SIZE];

    //announce communication partner
    client_ip = inet_ntoa(conn.remote_sa.sin_addr);
    client_port = ntohs(conn.remote_sa.sin_port);
    char userstring[50];
    snprintf(userstring, 50, "unknown (%s:%u)", client_ip, client_port);
    printf("new connection from %s:%d\n", client_ip, client_port);
    while((bytes_received = recv(conn.conn_fd, buf, BUF_SIZE, 0)) > 0) {
        printf("message receieved from client %d\n", conn.clientnum);
        fflush(stdout);

        //send it back
        send(conn.conn_fd, buf, bytes_received, 0);
    }
    char s[50];
    disconnectstring(s, userstring);
    close(conn.conn_fd);
    return NULL;
}

/*void *sendmessage(void *a) {
    //function to send message to all active clients
}*/

void newnickstring(char *s, char *new, char *old) {
    //TODO figure out more elegant solution for size
    size_t size = strlen(new) + strlen(old) + 25;
    snprintf(s, size, "User %s is now known as %s.\n", old, new);
}

void disconnectstring(char *s, char *user) {
    size_t size = strlen(user) + 25;
    snprintf(s, size, "User %s has disconnected.\n", user);
}
