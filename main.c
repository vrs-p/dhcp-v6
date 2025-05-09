#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "messages/router_advertisment/router_advertisement.h"
#include "server/init.h"

#define BUF_SIZE 1024

/**
 * @brief Initializes DHCP communication.
 *
 * This function sets up a raw socket for DHCP communication, binds it to an address,
 * and joins a multicast group. It then enters a loop where it listens for incoming
 * Router Solicitation messages and responds to them with a Router Advertisement message.
 *
 * @return NULL after closing the socket.
 */
void * init_dhcp_communication() {
    int sockfd;
    char buffer[BUF_SIZE];
    struct sockaddr_in6 dest, client;
    struct ipv6_mreq mreq;
    socklen_t client_len = sizeof(client);
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

/**
 * @brief Initializes a DHCPv6 server.
 *
 * This function initializes a DHCPv6 server by calling the init_dhcp_v6 function.
 * After the server is initialized, the function closes the socket and returns.
 *
 * @return NULL after closing the socket.
 */
void * init_dhcp_server() {
    int sock = init_dhcp_v6();
    close(sock);
    return 0;
}

/**
 * @brief Main function.
 *
 * This function creates new threads for both DHCP communication and the DHCPv6 server.
 * It then waits for both threads to finish their execution.
 *
 * @return 0 on successful execution.
 */
int main() {
    pthread_t /*dhcp_communication,*/ dhcp_server;

//    pthread_create(&dhcp_communication, NULL, init_dhcp_communication, NULL);
    pthread_create(&dhcp_server, NULL, init_dhcp_server, NULL);

//    pthread_join(dhcp_communication, NULL);
    pthread_join(dhcp_server, NULL);
    return 0;
}
