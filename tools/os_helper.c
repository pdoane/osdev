// ------------------------------------------------------------------------------------------------
// os_helper.c
// ------------------------------------------------------------------------------------------------

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// ------------------------------------------------------------------------------------------------
#define MYPORT "4950"
#define MAXBUFLEN 2048

// ------------------------------------------------------------------------------------------------
static void sockaddr_to_str(char* buf, size_t len, struct sockaddr *sa)
{
    const void* src;
    if (sa->sa_family == AF_INET)
    {
        src = &(((struct sockaddr_in*)sa)->sin_addr);
        inet_ntop(sa->sa_family, src, buf, len);
    }
}

// ------------------------------------------------------------------------------------------------
int main(void)
{
    // Get addresses
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4 only
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *servinfo;
    int rc = getaddrinfo(NULL, MYPORT, &hints, &servinfo);
    if (rc != 0)
    {
        fprintf(stderr, "Failed to get addresses: %s\n", gai_strerror(rc));
        return EXIT_FAILURE;
    }

    // Create socket
    int sockfd = -1;
    for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            perror("Failed to create socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            sockfd = -1;
            perror("Failed to bind socket");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (sockfd == -1)
    {
        fprintf(stderr, "Failed to bind socket\n");
        return EXIT_FAILURE;
    }

    // Listen for data
    printf("OS Helper ready\n");

    for (;;)
    {
        char buf[MAXBUFLEN];
        struct sockaddr_storage storage;
        struct sockaddr* addr = (struct sockaddr *)&storage;

        socklen_t addr_len = sizeof(storage);
        int rx_count = recvfrom(sockfd, buf, MAXBUFLEN , 0, addr, &addr_len);
        if (rx_count == -1)
        {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        char addr_str[INET6_ADDRSTRLEN];
        sockaddr_to_str(addr_str, sizeof(addr_str), addr);

        printf("%s: %s\n", addr_str, buf);
    }

    close(sockfd);

    return EXIT_SUCCESS;
}
