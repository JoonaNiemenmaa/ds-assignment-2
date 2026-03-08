#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "clients.c"
#include "../lib/string.c"

#define BACKLOG 10
#define BUFFER_SIZE 255

client_t *list = NULL;

typedef struct {
    client_t *client;
} args_t;

void *serve_client(void *arg) {
    args_t *args = (args_t *)arg;

    client_t *client = args->client;

    free(args);

    char buffer[BUFFER_SIZE] = "";
    int received_bytes = recv(client->sock, buffer, BUFFER_SIZE, 0);

    while (received_bytes > 0) {

        string_t msg = string(client->nickname);
        string_push(&msg, ": ");
        string_push(&msg, buffer);

        for (client_t *p = list; p; p = p->next) {
            int sent_bytes = 0;
            sent_bytes = send(p->sock, msg.str, msg.capacity, 0);
        }

        free(msg.str);

        received_bytes = recv(client->sock, buffer, BUFFER_SIZE, 0);
    }

    close(client->sock);
    list = clients_remove(list, client);

    return NULL;
}

int main(int argc, char **argv) {

    char *port = "3000";

    struct addrinfo request = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };
    struct addrinfo *address_info;

    int gai_code = getaddrinfo(NULL, port, &request, &address_info);
    if (gai_code != 0) {
        fprintf(stderr, "error: %s\n", gai_strerror(gai_code));
        exit(1);
    }

    int sock = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (sock == -1) {
        perror("error");
        exit(1);
    }

    /* The snippet below taken from the link below */
    /* https://beej.us/guide/bgnet/html/split/system-calls-or-bust.html#bind */
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
    char nickname_buffer[NICKNAME_LENGTH] = "";
    while ((connection = accept(sock, (struct sockaddr *)&their_address, &address_size)) > -1) {
        int received_bytes = recv(connection, nickname_buffer, NICKNAME_LENGTH, 0);

        if (received_bytes > 0) {
            pthread_t thread;
            client_t *client = create_client(thread, connection, nickname_buffer);

            int code = 0;
            if (list) {
                code = clients_append(list, client);
            } else {
                list = client;
            }

            if (code == -1) {
                free(client);
                close(connection);
                continue;
            }

            args_t *args = NULL;
            if ((args = (args_t *)malloc(sizeof(args_t))) == NULL) {
                perror("error");
                exit(1);
            }

            args->client = client;

            pthread_create(&thread, NULL, serve_client, args);
        }
    }

    freeaddrinfo(address_info);
    perror("error");
    close(sock);

    return 0;
}
