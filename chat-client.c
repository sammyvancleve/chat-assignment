/*
   chat-client.c
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#define BUF_SIZE 4096
#define TIME_STRING_SIZE 15

void *receive(void *conn_fd);
void gettime(char *s);

int main(int argc, char *argv[]) {
    char *dest_hostname, *dest_port;
    struct addrinfo hints, *res;
    char buf[BUF_SIZE];
    int conn_fd, n, rc;

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

    pthread_t rcvthread;
    pthread_create(&rcvthread, NULL, receive, &conn_fd);

    //while loop until user exits (ctrl-d)
    while((n = read(0, buf, BUF_SIZE)) > 0) {
        char *s;
        s = strtok(buf, "\n");
        send(conn_fd, s, n, 0);
    }
    //printf("conn broken\n");

    //send disconnect before exiting
    strcpy(buf, "/disconnect");
    send(conn_fd, buf, BUF_SIZE, 0);

    close(conn_fd);
}

void *receive(void *conn_fd) {
    int fd = *((int *)conn_fd);
    char buf[BUF_SIZE];
    while(recv(fd, buf, BUF_SIZE, 0) != 0) {
        char s[TIME_STRING_SIZE];
        gettime(s);
        printf("%s", s);
        puts(buf);
        fflush(stdout);
    }
    printf("disconnected from server\n");
    exit(0);
}

void gettime(char *s) {
    time_t currenttime;
    time(&currenttime);
    struct tm *local = localtime(&currenttime);
    int hr = local->tm_hour;
    int min = local->tm_min;
    int sec = local->tm_sec;
    snprintf(s, TIME_STRING_SIZE, "%d:%d:%d: ", hr, min, sec);
}
