//
// Created by student on 21.4.2024.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "dhcpv6_advertisment.h"

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

int read_client_id(uint8_t **duid, size_t *duid_len) {
    FILE *file = fopen("/etc/client-id", "r");
    if (!file) {
        perror("Failed to open /etc/client-id");
        return -1;
    }

    char hex_string[33]; // /etc/machine-id is 32 characters long
    if (fgets(hex_string, sizeof(hex_string), file) == NULL) {
        perror("Failed to read /etc/client-id");
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

void increment_ipv6_addr(struct in6_addr *addr) {
    for (int i = 15; i >= 0; --i) {
        if (addr->s6_addr[i]++ != 0xff) {
            break;
        }
    }
}

void send_dhcpv6_advertisement(struct sockaddr_in6 *client, char *solicit_data, int bytes_received, int dhcp_sock, struct in6_addr* address) {
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
    //for testing
    uint8_t *client_duid;
    size_t client_duid_len;
    if (read_client_id(&client_duid, &client_duid_len) != 0) {
        fprintf(stderr, "Error reading client ID\n");
        return;
    }
    client_identifier->hdr.l = htons(client_duid_len);
    memcpy(client_identifier->duid, client_duid, client_duid_len);

    memcpy(pointer, client_identifier, sizeof(struct opt_hdr) + htons(client_identifier->hdr.l));


//    IA definition
    // IAADDR
    IA_ADDR iaaddr;
    iaaddr.hdr.t = htons(DHCPV6_OPTION_IAADDR);
    iaaddr.hdr.l = htons(24);
    increment_ipv6_addr(address);
    iaaddr.addr = *address;
    iaaddr.preferred_lifetime = htonl(86400);  // example preferred lifetime
    iaaddr.valid_lifetime = htonl(172800);  // example valid lifetime
    // IANA
    IA_NA ia_na;
    ia_na.hdr.t = htons(DHCPV6_OPTION_IA_NA);
    ia_na.hdr.l = htons(12 + sizeof(struct opt_hdr) + ntohs(iaaddr.hdr.l));
//    offset = find_offset_option(solicit_data, 3, bytes_received);
    offset = 36;
    IA_NA* ia_na_c = (IA_NA*)(solicit_data + offset);
    ia_na.iaid = ia_na_c->iaid;
    ia_na.t1 = htonl(43200);  // T1 value
    ia_na.t2 = htonl(69120);  // T2 value
    ia_na.iaAddr = iaaddr;

    // Copy the IA_NA header first
    pointer = pointer + sizeof(struct opt_hdr) + ntohs(client_identifier->hdr.l);
    memcpy(pointer, &ia_na, sizeof(ia_na.hdr) + sizeof(ia_na.iaid) + sizeof(ia_na.t1) + sizeof(ia_na.t2));
    pointer += sizeof(ia_na.hdr) + sizeof(ia_na.iaid) + sizeof(ia_na.t1) + sizeof(ia_na.t2);
    msg_size += sizeof(ia_na.hdr) + sizeof(ia_na.iaid) + sizeof(ia_na.t1) + sizeof(ia_na.t2);

    // Then copy the IAADDR
    memcpy(pointer, &ia_na.iaAddr, sizeof(ia_na.iaAddr));
    pointer += sizeof(ia_na.iaAddr);
    msg_size += sizeof(ia_na.iaAddr);

    // DNS Recursive Name Server Option
    DNS_RECURSIVE_NAME_SERVER dns_servers;
    dns_servers.hdr.t = htons(23);  // Option code for DNS Recursive Name Server
    dns_servers.hdr.l = htons(sizeof(struct in6_addr));
    inet_pton(AF_INET6, "2001:4860:4860::8888", &dns_servers.addrs);

    memcpy(pointer, &dns_servers, sizeof(DNS_RECURSIVE_NAME_SERVER));
    pointer += sizeof(DNS_RECURSIVE_NAME_SERVER);
    msg_size += sizeof(DNS_RECURSIVE_NAME_SERVER);

    // Domain Search List Option
    DNS_SEARCH_LIST domain_search;
    domain_search.hdr.t = htons(24);  // Option code for Domain Search List
    const char* domain_name = "example.com";
    domain_search.domain_name_length = htons(strlen(domain_name));
    domain_search.hdr.l = htons(sizeof(domain_search.domain_name_length) + strlen(domain_name));
    memcpy(domain_search.domain_name, domain_name, strlen(domain_name));

    memcpy(pointer, &domain_search, sizeof(struct opt_hdr) + sizeof(domain_search.domain_name_length) + strlen(domain_name));
    pointer += sizeof(struct opt_hdr) + sizeof(domain_search.domain_name_length) + strlen(domain_name);
    msg_size += sizeof(struct opt_hdr) + sizeof(domain_search.domain_name_length) + strlen(domain_name);

//    create client address
    client->sin6_port = htons(546);
// Send UDP packet
    if (sendto(dhcp_sock, message, msg_size, 0, (struct sockaddr *)client, sizeof(*client)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}
