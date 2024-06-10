//
// Created by student on 21.4.2024.
//

#ifndef DHCP_V6_DHCPV6_ADVERTISMENT_H
#define DHCP_V6_DHCPV6_ADVERTISMENT_H

#include <netinet/in.h>

#include "../../structures/dhcpv6_struct/dhcpv6_struct.c"

#define DHCPV6_OPTION_IA_NA 3
#define DHCPV6_OPTION_IAADDR 5

// Identity Association for Non-temporary Addresses
typedef struct ia_na {
    struct opt_hdr hdr;     // option header -> type and length
    uint32_t iaid;          // IAID (Identity Assoctiation ID)
    uint32_t t1;            // T1 (time to start renewing)
    uint32_t t2;            // T2 (time to start rebinding)
} IA_NA;

// IA Address
typedef struct ia_address {
    struct opt_hdr hdr;
    struct in6_addr addr;           // assigned IPv6 address
    uint32_t preferred_lifetime;    // preferred lifetime of the address
    uint32_t valid_lifetime;        // valid lifetime of the address
} IA_ADDR;


void send_dhcpv6_advertisement(struct sockaddr_in6 *client, char *solicit_data, int bytes_received, int dhcp_sock);

#endif //DHCP_V6_DHCPV6_ADVERTISMENT_H
