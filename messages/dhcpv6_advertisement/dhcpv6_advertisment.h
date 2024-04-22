//
// Created by student on 21.4.2024.
//

#ifndef DHCP_V6_DHCPV6_ADVERTISMENT_H
#define DHCP_V6_DHCPV6_ADVERTISMENT_H

#include <netinet/in.h>

#include "../../structures/dhcpv6_struct/dhcpv6_struct.c"

void send_dhcpv6_advertisement(struct sockaddr_in6 *client, char *solicit_data, int bytes_received, int dhcp_sock);

#endif //DHCP_V6_DHCPV6_ADVERTISMENT_H
