//
// Created by student on 21.4.2024.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "dhcpv6_advertisment.h"

/**
 * Reads the machine ID from the /etc/machine-id file.
 * The machine ID is a unique identifier for the machine, represented as a hexadecimal string.
 * This function converts the hexadecimal string to a byte array.
 *
 * @param duid A pointer to a pointer for the byte array. This function will allocate memory for the byte array.
 * @param duid_len A pointer to a size_t where the length of the byte array will be stored.
 * @return 0 on success, -1 on failure (e.g., if the file could not be opened or read, or if memory allocation failed).
 */
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

/**
 * Reads the client ID from the /etc/client-id file.
 * The client ID is a unique identifier for the client, represented as a hexadecimal string.
 * This function converts the hexadecimal string to a byte array.
 *
 * @param duid A pointer to a pointer for the byte array. This function will allocate memory for the byte array.
 * @param duid_len A pointer to a size_t where the length of the byte array will be stored.
 * @return 0 on success, -1 on failure (e.g., if the file could not be opened or read, or if memory allocation failed).
 */
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

/**
 * Finds the offset of a specific option in a DHCPv6 message.
 *
 * @param data A pointer to the start of the DHCPv6 message.
 * @param option The option to find.
 * @param bytes_received The total number of bytes in the DHCPv6 message.
 * @return The offset of the option in the DHCPv6 message, or -1 if the option was not found.
 */
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

/**
 * Increments an IPv6 address by one.
 * This function handles carry-over: if a byte is 0xff and is incremented, it will wrap around to 0x00 and the next byte will be incremented.
 *
 * @param addr A pointer to the IPv6 address to increment.
 */
void increment_ipv6_addr(struct in6_addr *addr) {
    for (int i = 15; i >= 0; --i) {
        if (addr->s6_addr[i]++ != 0xff) {
            break;
        }
    }
}

/**
 * Sends a DHCPv6 Advertise or Reply message to a client.
 *
 * This function constructs a DHCPv6 message with the appropriate options,
 * including the server ID, client ID, IA_NA, IAADDR, DNS servers, and domain search list.
 * The type of message (Advertise or Reply) is determined by the 'type' parameter.
 *
 * @param client A pointer to a sockaddr_in6 structure containing the client's address.
 * @param solicit_data A pointer to the data received in the Solicit message from the client.
 * @param bytes_received The number of bytes received in the Solicit message.
 * @param dhcp_sock The socket descriptor for the DHCP socket.
 * @param address A pointer to an in6_addr structure containing the IPv6 address to be assigned to the client.
 * @param type The type of message to send (1 for Advertise, 2 for Reply).
 *
 * At the end of this function, it sends the constructed DHCPv6 message as a UDP packet to the client.
 */
void send_dhcpv6_adver_reply(struct sockaddr_in6 *client, char *solicit_data, int bytes_received, int dhcp_sock, struct in6_addr* address, int type) {
    struct msg_hdr* client_header = (struct msg_hdr*)solicit_data;
    int offset = find_offset_option(solicit_data, 1, bytes_received);
    struct opt_client_id* client_identifier = (struct opt_client_id*)(solicit_data + offset);

    char message[1024];
    size_t msg_size = 0;
    char* pointer = message;

    struct msg_hdr* header = (struct msg_hdr*)pointer;
    msg_size += sizeof(struct msg_hdr);
    if (type == 1) {
        header->type = 2;
    } else if (type == 3) {
        header->type = 7;
    } else {
        header->type = -1;
    }
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
    if (type == 1) {
        increment_ipv6_addr(address);
    }
    iaaddr.addr = *address;
    iaaddr.preferred_lifetime = htonl(86400);  // example preferred lifetime
    iaaddr.valid_lifetime = htonl(172800);  // example valid lifetime
    // IANA
    IA_NA ia_na;
    ia_na.hdr.t = htons(DHCPV6_OPTION_IA_NA);
    ia_na.hdr.l = htons(12 + sizeof(struct opt_hdr) + ntohs(iaaddr.hdr.l));
//    offset = find_offset_option(solicit_data, 3, bytes_received);
    if (type == 1) {
        offset = 36;
    } else if (type == 3) {
        offset = 61;
    } else {
        offset = -1;
    }
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
