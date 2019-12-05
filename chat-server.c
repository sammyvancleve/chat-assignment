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
#define NUM_CLIENTS 6
#define MAX_NICK_LEN 75

void *connhandler(void *threadid);
void sendmessage(char *message, char *user);
void disconnectstring(char *s, char *user);
void newuser(char *s, char *client_ip, uint16_t client_port);
void newconn(char *user);
void newnick(char *new, char *old);
void timestring(char *s);

struct threadconn {
    socklen_t addrlen;
    struct sockaddr_in remote_sa;
    int conn_fd;
    int active;
};

static struct threadconn client[NUM_CLIENTS];

pthread_mutex_t mutex;

int main(int arvc, char *argv[]) {
    char *listen_port;
    int listen_fd;
    struct addrinfo hints, *res;
    int rc;

    pthread_t client_threads[NUM_CLIENTS];

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
            client[i].addrlen = sizeof(client[i].remote_sa);
            client[i].conn_fd = accept(listen_fd, (struct sockaddr *) &client[i].remote_sa, &client[i].addrlen);
            pthread_create(&client_threads[i], NULL, connhandler, &client[i]);
	    printf("spun out thread\n");
        }
        for (int i = 0; i < NUM_CLIENTS; i++) {
            pthread_join(client_threads[i], NULL);
        }
    }
}

void *connhandler(void *threadid) {
    struct threadconn conn = *((struct threadconn *) threadid);
    int bytes_received;
    char buf[BUF_SIZE];

    //userinfo strings
    char *client_ip;
    uint16_t client_port;

    //announce communication partner
    client_ip = inet_ntoa(conn.remote_sa.sin_addr);
    client_port = ntohs(conn.remote_sa.sin_port);
    char userstring[MAX_NICK_LEN];
    newuser(userstring, client_ip, client_port);
    newconn(userstring);
    printf("new connection from %s:%d\n", client_ip, client_port);

    while((bytes_received = recv(conn.conn_fd, buf, BUF_SIZE, 0)) > 0) {
        fflush(stdout);
        if (strcmp("/disconnect", buf) == 0) {
            break;
        }
        if (strncmp("/nick", buf, 5) == 0) {
            char buf2[BUF_SIZE];
            strcpy(buf2, buf);
            char *s;
            s = strtok(buf2, " ");
            s = strtok(NULL, " ");
	    newnick(s, userstring);
        }
        pthread_mutex_lock(&mutex);
        sendmessage(buf, userstring);
        pthread_mutex_unlock(&mutex);
    }
    char s[MAX_NICK_LEN+25];
    disconnectstring(s, userstring);
    printf("user %s has disconnected\n", userstring);
    close(conn.conn_fd);
    return NULL;
}

void sendmessage(char *message, char *sender) {
    char s[BUF_SIZE];
    memset(s, 0, BUF_SIZE);
    snprintf(s, BUF_SIZE, "%s: %s\n", sender, message);
    for (int i = 0; i < NUM_CLIENTS; i++) {
        send(client[i].conn_fd, s, BUF_SIZE, 0);
    }
}

void newconn(char *user) {
    char s[BUF_SIZE];
    snprintf(s, BUF_SIZE, "new connection from %s", user);
    sendmessage(s, "");
}

void newuser(char *s, char *client_ip, uint16_t client_port) {
    snprintf(s, MAX_NICK_LEN, "User unknown (%s:%u)", client_ip, client_port);
}

void newnick(char *new, char *old) {
    char s[BUF_SIZE];
    snprintf(s, BUF_SIZE, "%s is now known as %s.", old, new);
    printf("%s", s);
    sendmessage(s, "");
    memset(old, 0, strlen(old));
    strcpy(old, new);
}

void disconnectstring(char *s, char *user) {
    size_t size = strlen(user) + 25;
    snprintf(s, size, "User %s has disconnected.\n", user);
    sendmessage(s, "");
}
