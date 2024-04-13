//
// Created by student on 13.4.2024.
//

#define INTER "enp0s8"
#define DHCP_PORT_RECV 547

#include <net/if.h>
#include "init.h"

int init_dhcp_v6() {
    char buffer[1024];
    int dhcp_socket;
    struct sockaddr_in6 my_addr, client;
    struct ipv6_mreq multicast_group;
    socklen_t client_len;

    // Create a raw socket
    dhcp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (dhcp_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin6_family = AF_INET6;
    my_addr.sin6_port = htons(DHCP_PORT_RECV);
    inet_pton(AF_INET6, "::", &my_addr.sin6_addr);
    if (bind (dhcp_socket,(struct sockaddr *) &my_addr,sizeof (my_addr)) < 0)
        exit(EXIT_FAILURE);

    // Join the link-local all nodes multicast group
    inet_pton (AF_INET6,"ff02::1:2",&multicast_group.ipv6mr_multiaddr);
    multicast_group.ipv6mr_interface = if_nametoindex(INTER);
    if (setsockopt (dhcp_socket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *) &multicast_group, sizeof (multicast_group)) < 0)
        exit(EXIT_FAILURE);


    int bytes_received;
    memset(buffer, 0, 1024);
    bytes_received = recvfrom(dhcp_socket, buffer, 1024, 0, (struct sockaddr *)&client, &client_len);
    printf("RS received");

    unsigned char message_type = buffer[0];
    if (message_type == 1) {
        printf("Solicit message received");
    } else {
        printf("Something else received");
    }

//    close(dhcp_socket);
    return dhcp_socket;
}
