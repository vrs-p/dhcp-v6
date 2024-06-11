//
// Created by student on 21.4.2024.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dhcpv6_advertisment.h"

int find_offset_option(char *data, uint16_t option, int bytes_received) {
    size_t current_offset = sizeof(struct msg_hdr) + 1;
    while (current_offset <= bytes_received) {
        struct opt_hdr* option_hdr = (struct opt_hdr*)(data + current_offset);
        if (option_hdr->t == option){
            return (int)current_offset;
        }
        current_offset += sizeof(struct opt_hdr) + option_hdr->l + 1;
    }
    return -1;
}

void send_dhcpv6_advertisement(struct sockaddr_in6 *client, char *solicit_data, int bytes_received, int dhcp_sock) {
    struct msg_hdr* client_header = (struct msg_hdr*)solicit_data;
    int offset = find_offset_option(solicit_data, 1, bytes_received);
    struct opt_client_id* client_identifier = (struct opt_client_id*)(solicit_data + offset);

    char message[1024];
    size_t msg_size = 0;
    char* pointer = message;

    struct msg_hdr* header = (struct msg_hdr*)pointer;
    msg_size += sizeof(struct msg_hdr);
    header->type = 2;
    header->xid = client_header->xid;

//    server identifier TODO: needs to be generated, now it's only copy paste from client
    pointer = pointer + sizeof(struct msg_hdr);
    msg_size += sizeof(struct opt_hdr) + client_identifier->hdr.l;
    client_identifier->hdr.t = 2;
    memcpy(pointer, client_identifier, sizeof(struct opt_hdr) + client_identifier->hdr.l);

//    client identifier
    pointer = pointer + sizeof(struct msg_hdr);
    msg_size += sizeof(struct opt_hdr) + client_identifier->hdr.l;
    client_identifier->hdr.t = htons(1);
    client_identifier->hdr.l = htons(client_identifier->hdr.l);

    memcpy(pointer, client_identifier, sizeof(struct opt_hdr) + htons(client_identifier->hdr.l));

//    create client address
    client->sin6_port = htons(546);
// Send UDP packet
    if (sendto(dhcp_sock, message, msg_size, 0, (struct sockaddr *)client, sizeof(*client)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}
