#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/* From here you get the byte order conversion functions */
#include <arpa/inet.h>

int main(int argc, char** argv) {

    char *host = "localhost";
    char *port = "3000";

    struct addrinfo request;

    memset(&request, 0, sizeof(struct addrinfo));

    request.ai_flags = AI_PASSIVE;
    request.ai_family = AF_UNSPEC;
    request.ai_socktype = SOCK_STREAM;

    struct addrinfo *addrinfo;

    int addrinfo_code = getaddrinfo(host, port, &request, &addrinfo);
    if (addrinfo_code > 0) {
        printf("error: %s\n", gai_strerror(addrinfo_code));
        exit(1);
    }



    return 0;
}
