#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>

#include <pthread.h>

#include "clients.c"
#include "../lib/string.c"

#define BACKLOG 10
#define BUFFER_SIZE 255

client_t *list = NULL;

pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    client_t *client;
} args_t;

char *skip_whitespace(char *p) {
    while (*p != ' ') {
        p++;
    }
    return p;
}

void *serve_client(void *arg) {
    args_t *args = (args_t *)arg;

    client_t *client = args->client;

    free(args);

    pthread_mutex_lock(&list_lock);
    for (client_t *p = list; p; p = p->next) {
        if (p != client) {
            string_t msg = string(client->nickname);

            string_push(&msg, " connected");

            pthread_mutex_lock(&p->sock_lock);
            send(p->sock, msg.str, msg.capacity, 0);
            pthread_mutex_unlock(&p->sock_lock);

            free(msg.str);
        }
    }
    pthread_mutex_unlock(&list_lock);

    const int pfds_size = 1;
    struct pollfd pfds[pfds_size];
    pfds[0].fd = client->sock;
    pfds[0].events = POLLIN;
    const int timeout = -1;

    char buffer[BUFFER_SIZE] = "";

    poll(pfds, pfds_size, timeout);

    int pollin = pfds[0].revents & POLLIN;
    int pollhup = pfds[0].revents & POLLHUP;

    if (pollin) {
        pthread_mutex_lock(&client->sock_lock);
        int received_bytes = recv(client->sock, buffer, BUFFER_SIZE, 0);
        pthread_mutex_unlock(&client->sock_lock);


        bool quit = false;
        while (received_bytes > 0 && !pollhup && !quit) {

            if (buffer[0] == '/' && strlen(buffer) >= 2) {
                int len = 0;
                char *p;
                char command = buffer[1];
                char peer_nickname[NICKNAME_LENGTH] = "";
                switch (command) {
                    case 'q':
                        quit = true;
                        break;
                    case 'm':
                        if (strlen(buffer) >= 4) {
                            p = &buffer[3];
                            while (*p != ' ' && *p != '\0') {
                                p++;
                                len++;
                            }
                            memcpy(peer_nickname, &buffer[3], len);
                        }

                        pthread_mutex_lock(&list_lock);
                        client_t *peer = clients_get_by_nickname(list, peer_nickname);
                        pthread_mutex_unlock(&list_lock);

                        if (peer) {
                            string_t msg = string(client->nickname);

                            string_push(&msg, ": ");
                            string_push(&msg, buffer + len + 4);

                            pthread_mutex_lock(&client->sock_lock);
                            send(peer->sock, msg.str, msg.capacity, 0);
                            send(client->sock, msg.str, msg.capacity, 0);
                            pthread_mutex_unlock(&client->sock_lock);

                            free(msg.str);
                        } else {
                            pthread_mutex_lock(&client->sock_lock);
                            send(client->sock, "no such peer", 13, 0);
                            pthread_mutex_unlock(&client->sock_lock);
                        }

                        break;
                    case 'c':
                        break;
                    default:
                        pthread_mutex_lock(&client->sock_lock);
                        send(client->sock, "command not recognized", 23, 0);
                        pthread_mutex_unlock(&client->sock_lock);
                }

            } else {
                string_t msg = string(client->nickname);
                string_push(&msg, ": ");
                string_push(&msg, buffer);

                pthread_mutex_lock(&list_lock);
                for (client_t *p = list; p; p = p->next) {
                    pthread_mutex_lock(&p->sock_lock);
                    send(p->sock, msg.str, msg.capacity, 0);
                    pthread_mutex_unlock(&p->sock_lock);
                }
                pthread_mutex_unlock(&list_lock);

                free(msg.str);
            }

            if (!quit) {
                poll(pfds, pfds_size, timeout);

                pollin = pfds[0].revents & POLLIN;
                pollhup = pfds[0].revents & POLLHUP;

                if (pollin) {
                    pthread_mutex_lock(&client->sock_lock);
                    received_bytes = recv(client->sock, buffer, BUFFER_SIZE, 0);
                    pthread_mutex_unlock(&client->sock_lock);
                }
            }
        }
    }

    pthread_mutex_lock(&list_lock);
    for (client_t *p = list; p; p = p->next) {
        string_t msg = string(client->nickname);

        string_push(&msg, " disconnected");

        pthread_mutex_lock(&p->sock_lock);
        send(p->sock, msg.str, msg.capacity, 0);
        pthread_mutex_unlock(&p->sock_lock);

        free(msg.str);
    }
    pthread_mutex_unlock(&list_lock);

    close(client->sock);
    pthread_mutex_destroy(&client->sock_lock);

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
            pthread_mutex_lock(&list_lock);
            if (list) {
                code = clients_append(list, client);
            } else {
                list = client;
            }
            pthread_mutex_unlock(&list_lock);

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

    pthread_mutex_destroy(&list_lock);
    freeaddrinfo(address_info);
    perror("error");
    close(sock);

    return 0;
}
