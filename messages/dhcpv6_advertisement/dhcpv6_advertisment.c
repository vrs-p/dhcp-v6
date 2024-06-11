//
// Created by student on 21.4.2024.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dhcpv6_advertisment.h"

#include <arpa/inet.h>

int read_machine_id(uint8_t **duid, size_t *duid_len) {
    FILE *file = fopen("/etc/machine-id", "r");
    if (!file) {
        perror("Failed to open /etc/machine-id");
        return -1;
    }

    char hex_string[33]; // /etc/machine-id is 32 characters long
    if (fgets(hex_string, sizeof(hex_string), file) == NULL) {
        perror("Failed to read /etc/machine-id");
        fclose(file);
        return -1;
    }
    fclose(file);

    // Convert hex string to byte array
    *duid_len = strlen(hex_string) / 2;
    *duid = malloc(*duid_len);
    if (!*duid) {
        perror("Failed to allocate memory for DUID");
        return -1;
    }

    for (size_t i = 0; i < *duid_len; i++) {
        sscanf(&hex_string[i * 2], "%2hhx", &(*duid)[i]);
    }

    return 0;
}

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

// Prepare server identifier
    struct opt_server_id server_identifier;
    server_identifier.hdr.t = htons(2);

    uint8_t *server_duid;
    size_t server_duid_len;

    if (read_machine_id(&server_duid, &server_duid_len) != 0) {
        fprintf(stderr, "Error reading machine ID\n");
        return;
    }

    server_identifier.hdr.l = htons(server_duid_len);
    memcpy(server_identifier.duid, server_duid, server_duid_len);

    // Copy server identifier to message
    pointer = pointer + sizeof(struct msg_hdr);
    memcpy(pointer, &server_identifier, sizeof(struct opt_hdr) + server_duid_len);
    msg_size += sizeof(struct opt_hdr) + server_duid_len;


//    client identifier
    pointer = pointer + sizeof(struct opt_hdr) + server_duid_len;
    msg_size += sizeof(struct opt_hdr) + client_identifier->hdr.l;
    client_identifier->hdr.t = htons(1);
    client_identifier->hdr.l = htons(client_identifier->hdr.l);
    memcpy(pointer, client_identifier, sizeof(struct opt_hdr) + htons(client_identifier->hdr.l));


//    IA definition
    // IAADDR
    IA_ADDR iaaddr;
    iaaddr.hdr.t = htons(DHCPV6_OPTION_IAADDR);
    iaaddr.hdr.l = htons(24);
    inet_pton(AF_INET6, "2001:db8::1", &iaaddr.addr);  // example IPv6 address
    iaaddr.preferred_lifetime = htonl(86400);  // example preferred lifetime
    iaaddr.valid_lifetime = htonl(172800);  // example valid lifetime
    // IANA
    IA_NA ia_na;
    ia_na.hdr.t = htons(DHCPV6_OPTION_IA_NA);
    ia_na.hdr.l = htons(12 + sizeof(struct opt_hdr) + iaaddr.hdr.l);
    ia_na.iaid = htonl(0x12345678);  // example IAID
    ia_na.t1 = htonl(43200);  // T1 value
    ia_na.t2 = htonl(69120);  // T2 value
    memcpy(&ia_na.iaAddr, &iaaddr, sizeof(struct opt_hdr) + 24);

    pointer = pointer + sizeof(struct opt_hdr) + client_identifier->hdr.l;
    msg_size += sizeof(struct opt_hdr) + 40;
    memcpy(pointer, &ia_na, sizeof(struct opt_hdr) + 40);

//    create client address
    client->sin6_port = htons(546);
// Send UDP packet
    if (sendto(dhcp_sock, message, msg_size, 0, (struct sockaddr *)client, sizeof(*client)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}
