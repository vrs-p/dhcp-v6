//
// Created by student on 21.4.2024.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dhcpv6_advertisment.h"

#include <arpa/inet.h>

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
    pointer = pointer + sizeof(struct opt_hdr) + client_identifier->hdr.l;
    msg_size += sizeof(struct opt_hdr) + client_identifier->hdr.l;
    client_identifier->hdr.t = 1;
    memcpy(pointer, client_identifier, sizeof(struct opt_hdr) + client_identifier->hdr.l);

//    IA definition
    // IANA
    IA_NA ia_na;
    ia_na.hdr.t = DHCPV6_OPTION_IA_NA;
    ia_na.hdr.l = sizeof(IA_NA) - sizeof(struct opt_hdr) + sizeof(IA_ADDR);
    ia_na.iaid = htonl(0x12345678);  // example IAID
    ia_na.t1 = htonl(0);  // T1 value
    ia_na.t2 = htonl(0);  // T2 value

    memcpy(pointer, &ia_na, sizeof(IA_NA));
    pointer += sizeof(IA_NA);
    msg_size += sizeof(IA_NA);

    // IAADDR
    IA_ADDR iaaddr;
    iaaddr.hdr.t = DHCPV6_OPTION_IAADDR;
    iaaddr.hdr.l = sizeof(IA_ADDR) - sizeof(struct opt_hdr);
    inet_pton(AF_INET6, "2001:db8::1", &iaaddr.addr);  // example IPv6 address
    iaaddr.preferred_lifetime = htonl(3600);  // example preferred lifetime
    iaaddr.valid_lifetime = htonl(7200);  // example valid lifetime

    memcpy(pointer, &iaaddr, sizeof(IA_ADDR));
    pointer += sizeof(IA_ADDR);
    msg_size += sizeof(IA_ADDR);

//    create client address
    client->sin6_port = htons(546);
// Send UDP packet
    if (sendto(dhcp_sock, message, msg_size, 0, (struct sockaddr *)client, sizeof(*client)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}
