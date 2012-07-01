// ------------------------------------------------------------------------------------------------
// tools/os_helper.c
// ------------------------------------------------------------------------------------------------

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// ------------------------------------------------------------------------------------------------
#define PORT 4950
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
    struct sockaddr_storage storage;
    struct sockaddr* addr = (struct sockaddr *)&storage;

    // Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1)
    {
        perror("Failed to create socket");
        return EXIT_FAILURE;
    }

    // Enable broadcast
    int val = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)))
    {
        perror("Failed to enable broadcast");
        return EXIT_FAILURE;
    }

    // Bind
    struct sockaddr_in* ipv4_addr = (struct sockaddr_in*)addr;
    ipv4_addr->sin_family = AF_INET;
    ipv4_addr->sin_port = htons(PORT);
    ipv4_addr->sin_addr.s_addr = 0;

    if (bind(sockfd, addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("Failed to bind socket");
        return EXIT_FAILURE;
    }

    // Listen for data
    printf("OS Helper ready\n");

    for (;;)
    {
        char buf[MAXBUFLEN];

        socklen_t addr_len = sizeof(storage);
        int rx_count = recvfrom(sockfd, buf, MAXBUFLEN , 0, addr, &addr_len);
        if (rx_count == -1)
        {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        char addr_str[INET6_ADDRSTRLEN];
        sockaddr_to_str(addr_str, sizeof(addr_str), addr);

        printf("%s", buf);
    }

    close(sockfd);

    return EXIT_SUCCESS;
}
