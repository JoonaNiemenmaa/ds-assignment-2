#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pthread.h>

#define BACKLOG 10
#define BUFFER_SIZE 255

typedef struct thread_args {
    int sock;
} thread_args_t;

void *serve_client(void *argv) {
    thread_args_t *args = (thread_args_t *)argv;

    int sock = args->sock;
    uint8_t buffer[BUFFER_SIZE] = "";
    int received_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
    while (received_bytes > 0) {
        printf("%s", buffer);
        received_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
    }

    close(sock);
    return NULL;
}

int main(int argc, char **argv) {

    char *port = "3000";

    struct addrinfo request = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_INET6,
        .ai_socktype = SOCK_STREAM
    };
    struct addrinfo *address_info;

    int gai_code = getaddrinfo(NULL, port, &request, &address_info);
    if (gai_code > 0) {
        fprintf(stderr, "error: %s\n", gai_strerror(gai_code));
        exit(1);
    }

    int sock = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (sock == -1) {
        perror("error");
        exit(1);
    }

    int yes=1;
    // lose the pesky "Address already in use" error message
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    if (bind(sock, address_info->ai_addr, address_info->ai_addrlen) == -1) {
        perror("error");
        exit(1);
    }

    if (listen(sock, BACKLOG) == -1) {
        perror("error");
        exit(1);
    }

    struct sockaddr_storage their_address;
    socklen_t address_size =  sizeof(their_address);

    int connection = 0;
    while ((connection = accept(sock, (struct sockaddr *)&their_address, &address_size)) > -1) {
        thread_args_t args = {
            .sock = connection
        };
        pthread_t thread;
        pthread_create(&thread, NULL, serve_client, &args);
    }

    freeaddrinfo(address_info);
    perror("error");
    close(sock);

    return 0;
}
