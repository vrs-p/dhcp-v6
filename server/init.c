//
// Created by student on 13.4.2024.
//

#define INTER "enp0s8"
#define DHCP_PORT_RECV 547

#include <net/if.h>
#include "init.h"
#include "../messages/dhcpv6_advertisement/dhcpv6_advertisment.h"

int init_dhcp_v6() {
    char buffer[1024];
    int dhcp_socket;
    struct sockaddr_in6 my_addr, client;
    struct ipv6_mreq multicast_group;
    socklen_t client_len = sizeof(client);

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
    int running = 1;
    int serving_req = 0;
    struct msg_hdr* served_req = malloc(sizeof(struct msg_hdr));
    memset(buffer, 0, 1024);

    while (running) {
        bytes_received = recvfrom(dhcp_socket, buffer, 1024, 0, (struct sockaddr *)&client, &client_len);
        if (bytes_received == -1) {
            perror("recvfrom");
            running = 0;
            continue;
        }

        struct msg_hdr* current_req = (struct msg_hdr*)buffer;
        char data[bytes_received];
        memcpy(data, buffer, bytes_received);

        if (serving_req == 0 && current_req->type == 1) {
            printf("Solicit message received");
            serving_req = 1;
            memcpy(served_req, current_req, sizeof(struct msg_hdr));
            send_dhcpv6_advertisement(&client, data, bytes_received, dhcp_socket);
        } else if (serving_req == 1 && current_req->type == 3 && memcmp(served_req, current_req, sizeof(struct msg_hdr)) == 0) {
            printf("Request message received");
            serving_req = 0;
            memset(served_req, 0, sizeof(struct msg_hdr));
        }
    }

    free(served_req);
    return dhcp_socket;
}
