#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <stdint.h>
#include <stdbool.h>

#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include <pthread.h>

#include <ncurses.h>
#include "../lib/string.c"

#define ARGS 2
#define MSG_SIZE 255

pthread_mutex_t sock_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ncurses_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int sock;
    WINDOW *win;
} arg_t;

void *run_rec_thread(void *arg) {
    arg_t *args = ((arg_t *) arg);

    int sock = args->sock;
    WINDOW *win_chat = args->win;
    int max_x, max_y;
    getmaxyx(win_chat, max_y, max_x);

    const int pfds_size = 1;
    struct pollfd pfds[pfds_size];
    pfds[0].fd = sock;
    pfds[0].events = POLLIN;
    const int timeout = -1;

    char rec_buffer[MSG_SIZE] = "";
    int received_bytes = -1;

    poll(pfds, pfds_size, timeout);

    int pollin_happened = pfds[0].revents & POLLIN;

    if (pollin_happened) {

        pthread_mutex_lock(&sock_lock);
        received_bytes = recv(sock, rec_buffer, MSG_SIZE, 0);
        pthread_mutex_unlock(&sock_lock);

        if (received_bytes > 0) {
            do {
                pthread_mutex_lock(&ncurses_lock);

                curs_set(0); // Set cursor to be invisible
                wprintw(win_chat, "%s\n", rec_buffer);
                int cy = getcury(win_chat);
                if (cy == max_y - 1) {
                    wmove(win_chat, 0, 0);
                    wdeleteln(win_chat);
                    wmove(win_chat, cy - 1, 0);
                }
                wrefresh(win_chat);

                pthread_mutex_unlock(&ncurses_lock);

                poll(pfds, pfds_size, timeout);

                pollin_happened = pfds[0].revents & POLLIN;

                if (pollin_happened) {
                    pthread_mutex_lock(&sock_lock);
                    received_bytes = recv(sock, rec_buffer, MSG_SIZE, 0);
                    pthread_mutex_unlock(&sock_lock);
                }

            } while (received_bytes > 0);
        }
    }

    pthread_mutex_lock(&ncurses_lock);

    wclear(win_chat);
    wprintw(win_chat, "server closed connection\n");
    wrefresh(win_chat);

    pthread_mutex_unlock(&ncurses_lock);

    return NULL;
}

int main(int argc, char** argv) {

    if (argc < 2) {
        fprintf(stderr, "usage: client nickname host\n");
        exit(1);
    }

    char *nickname = argv[1];
    char *host = argv[2];

    char *port = "3000";

    struct addrinfo request = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *address_info;

    int gai_code = getaddrinfo(host, port, &request, &address_info);
    if (gai_code != 0) {
        fprintf(stderr, "error: %s\n", gai_strerror(gai_code));
        exit(1);
    }

    int sock = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (sock == -1) {
        perror("error");
        exit(1);
    }

    if (connect(sock, address_info->ai_addr, address_info->ai_addrlen) == -1) {
        perror("error");
        exit(1);
    }

    int sent_bytes = send(sock, nickname, strlen(nickname) + 1, 0);

    if (sent_bytes == -1) {
        perror("error");
        close(sock);
        exit(1);
    }

    fcntl(sock, F_SETFL, O_NONBLOCK);

    initscr();

    int y, x;
    getmaxyx(stdscr, y, x);
    WINDOW *win_chat = newwin(y - 2, x, 0, 0);
    WINDOW *win_input = newwin(1, x, y - 2, 0);


    pthread_mutex_lock(&ncurses_lock);

    wprintw(win_chat, "connected to '%s' successfully as '%s'\n", host, nickname);
    wrefresh(win_chat);

    wclear(win_input);
    wprintw(win_input, "%s: \n", nickname);
    wrefresh(win_input);

    pthread_mutex_unlock(&ncurses_lock);

    pthread_t rec_thread;
    arg_t arg = {
        .sock = sock,
        .win = win_chat
    };
    pthread_create(&rec_thread, NULL, run_rec_thread, &arg);

    string_t msg = string("");
    while (sent_bytes != -1) {
        int input = wgetch(win_input);
        if (isprint(input)) {
            string_push_ch(&msg, input);
        } else {
            switch (input) {
                case 10: // ENTER
                    pthread_mutex_lock(&sock_lock);
                    sent_bytes = send(sock, msg.str, msg.capacity, 0);
                    pthread_mutex_unlock(&sock_lock);
                    string_clear(msg);
                    break;
                case 127: // DELETE
                    string_pop_ch(msg);
                    break;
            }
        }


        pthread_mutex_lock(&ncurses_lock);

        wclear(win_input);
        wprintw(win_input, "%s: %s\n", nickname, msg.str);
        wrefresh(win_input);

        pthread_mutex_unlock(&ncurses_lock);
    }

    free(msg.str);
    endwin();
    close(sock);
    pthread_mutex_destroy(&sock_lock);
    pthread_mutex_destroy(&ncurses_lock);
    freeaddrinfo(address_info);

    printf("disconnected\n");

    return 0;
}
