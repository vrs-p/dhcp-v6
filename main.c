#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "messages/router_advertisment/router_advertisement.h"

#define BUF_SIZE 1024

int main() {
    int sockfd;
    char buffer[BUF_SIZE];
    struct sockaddr_in6 dest, client;
    struct ipv6_mreq mreq;
    socklen_t client_len;
    int bytes_received;


    // Create a raw socket
    sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

//     BIND on all addresses
    memset(&dest, 0, sizeof(dest));
    dest.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::", &dest.sin6_addr);
    if (bind (sockfd,(struct sockaddr *) &dest,sizeof (dest)) < 0)
        exit(EXIT_FAILURE);

    // Join the link-local all nodes multicast group
    inet_pton (AF_INET6,"ff02::2",&mreq.ipv6mr_multiaddr);
    mreq.ipv6mr_interface = if_nametoindex(INTERFACE_NAME);
    if (setsockopt (sockfd,IPPROTO_IPV6,IPV6_ADD_MEMBERSHIP,(char *) &mreq,sizeof (mreq)) < 0)
        exit(EXIT_FAILURE);

    // Main loop to receive and respond to Router Solicitation messages
    int response_was_sent = 0;
    while (!response_was_sent) {
        // Receive Router Solicitation message from client
        memset(buffer, 0, BUF_SIZE);
        bytes_received = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&client, &client_len);
        if (bytes_received == -1) {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        if (bytes_received > 0)
        {
            printf("Router Solicitation received from: %s\n", inet_ntop(AF_INET6, &client.sin6_addr, buffer, INET6_ADDRSTRLEN));
            // Send Router Advertisement message in response
            send_router_advertisement_response();
            printf("Router Advertisement sent\n");
            response_was_sent = 1;
        }
    }

    close(sockfd);
    return 0;
}
