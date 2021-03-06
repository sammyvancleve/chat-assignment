/*
   chat-server.c
 */

#include <stdlib.h>
#include <time.h>
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
#define MAX_NICK_LEN 75

void *connhandler(void *structaddr);
void sendmessage(char *message, char *user);
void disconnectstring(char *user);
void newconn(char *user);
void newnick(char *new, char *old);

struct threadconn {
    socklen_t addrlen;
    struct sockaddr_in remote_sa;
    int conn_fd;
    void *prev;
    void *next;
};

//pointers to nodes in doubly linked list
static void *first;
static void *last;

pthread_mutex_t mutex;

int main(int arvc, char *argv[]) {
    char *listen_port;
    int listen_fd;
    struct addrinfo hints, *res;
    int rc;

    listen_port = argv[1];

    if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket: ");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if((rc = getaddrinfo(NULL, listen_port, &hints, &res)) != 0) {
        perror("getaddrinfo failed: ");
        exit(1);
    }

    if (bind(listen_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind: ");
        exit(1);
    }

    //start listening
    if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen: ");
        exit(1);
    }

    //infinite loop accepting new connections and handling them
    while(1) {
        //accept new connection (will block until one appears)
        pthread_t clientthread;
        void *structaddr = malloc(sizeof(struct threadconn));
        struct threadconn *client= structaddr;

        pthread_mutex_lock(&mutex);
        //add node to doubly linked list
        if (last == NULL && first == NULL) {
            //if only one client is connected...
            last = structaddr;
            first = structaddr;
            client->prev = NULL;
            client->next = NULL;
        } else {
            client->prev = last;
            client->next = NULL;
            struct threadconn *laststruct = last;
            laststruct->next = structaddr;
            last = structaddr;
        }
        pthread_mutex_unlock(&mutex);

        client->addrlen = sizeof(client->remote_sa);
        client->conn_fd = accept(listen_fd, (struct sockaddr *) &client->remote_sa, &client->addrlen);
        pthread_create(&clientthread, NULL, connhandler, client);
    }
}

void *connhandler(void *structaddr) {
    struct threadconn *conn = (struct threadconn *) structaddr;
    int bytes_received;
    char buf[BUF_SIZE];

    //userinfo strings
    char *client_ip;
    uint16_t client_port;

    //announce communication partner
    client_ip = inet_ntoa(conn->remote_sa.sin_addr);
    client_port = ntohs(conn->remote_sa.sin_port);
    char userstring[MAX_NICK_LEN];
    snprintf(userstring, MAX_NICK_LEN, "User unknown (%s:%u)", client_ip, client_port);
    newconn(userstring);
    printf("new connection from %s:%d\n", client_ip, client_port);

    while((bytes_received = recv(conn->conn_fd, buf, BUF_SIZE, 0)) > 0) {
        fflush(stdout);
        if (strncmp("/nick ", buf, 6) == 0) {
            char buf2[BUF_SIZE];
            strcpy(buf2, buf);
            char *nickname;
            //parse buf to find nick
            nickname = strtok(buf2, " ");
            nickname = strtok(NULL, " ");
            newnick(nickname, userstring);
            continue;
        }
        pthread_mutex_lock(&mutex);
        sendmessage(buf, userstring);
        pthread_mutex_unlock(&mutex);
    }
    disconnectstring(userstring);
    printf("user %s has disconnected\n", userstring);
    if (close(conn->conn_fd) == -1) {
        perror("error closing file descriptor");
    }

    //remove node from doubly linked list after connection closed
    pthread_mutex_lock(&mutex);
    if (first == structaddr) {
        if (conn->next == NULL) {
            first = NULL;
            last = NULL;
        } else {
            first = conn->next;
            struct threadconn *nextstruct = conn->next;
            nextstruct->prev= NULL;
        }
    } else if (last == structaddr) {
        last = conn->prev;
        struct threadconn *prevstruct = conn->prev;
        prevstruct->next = NULL;
    } else {
        struct threadconn *prevstruct = conn->prev;
        struct threadconn *nextstruct = conn->next;
        prevstruct->next = conn->next;
        nextstruct->prev = conn->prev;
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void sendmessage(char *message, char *sender) {
    char s[BUF_SIZE];
    if (sender != NULL) {
        snprintf(s, BUF_SIZE, "%s: %s", sender, message);
    } else {
        snprintf(s, BUF_SIZE, "%s", message);
    }
    void *sendaddr = first;
    while (sendaddr != NULL) {
        struct threadconn *sendclient = sendaddr;
        send(sendclient->conn_fd, s, BUF_SIZE, 0);
        sendaddr = sendclient->next;
    }
}

void newconn(char *user) {
    char s[BUF_SIZE];
    snprintf(s, BUF_SIZE, "new connection from %s", user);
    sendmessage(s, NULL);
}

void newnick(char *new, char *old) {
    char s[BUF_SIZE];
    snprintf(s, BUF_SIZE, "%s is now known as %s.", old, new);
    printf("%s\n", s);
    sendmessage(s, NULL);
    memset(old, 0, strlen(old));
    strcpy(old, new);
}

void disconnectstring(char *user) {
    char s[BUF_SIZE];
    snprintf(s, BUF_SIZE, "%s has disconnected.", user);
    sendmessage(s, NULL);
}
