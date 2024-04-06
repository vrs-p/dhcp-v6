//
// Created by student on 6.4.2024.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h>
#include <net/if.h>

#ifndef DHCPV6_SERVER_ROUTER_ADVERTISEMENT_H
#define DHCPV6_SERVER_ROUTER_ADVERTISEMENT_H

#define MULTICAST_ADDR "ff02::1"
#define INTERFACE_NAME "enp0s8"

unsigned short checksum(void *b, int len);

void send_router_advertisement_response();

#endif //DHCPV6_SERVER_ROUTER_ADVERTISEMENT_H
