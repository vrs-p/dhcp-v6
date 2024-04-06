//
// Created by student on 6.4.2024.
//

#include "router_advertisement.h"

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *) buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void send_router_advertisement_response() {
    // Create a raw socket
    int sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set up sockaddr_in6 structure for the multicast address
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = 0; // Not used in raw sockets
    inet_pton(AF_INET6, MULTICAST_ADDR, &addr.sin6_addr);

    // Bind the socket to the interface
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, INTERFACE_NAME, IFNAMSIZ - 1);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("setsockopt");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Prepare ICMPv6 Router Advertisement message
    char packet[1024];
    memset(packet, 0, sizeof(packet));
    struct nd_router_advert *ra_hdr = (struct nd_router_advert *)packet;
    ra_hdr->nd_ra_hdr.icmp6_type = ND_ROUTER_ADVERT; // Router Advertisement
    ra_hdr->nd_ra_hdr.icmp6_code = 0;
    ra_hdr->nd_ra_hdr.icmp6_cksum = 0; // Checksum will be calculated later
    ra_hdr->nd_ra_curhoplimit = 64; // Current Hop Limit
    ra_hdr->nd_ra_flags_reserved = 0x80; //ND_RA_FLAG_MANAGED
    ra_hdr->nd_ra_router_lifetime = htons(1800); // Router Lifetime: 1800 seconds
    ra_hdr->nd_ra_reachable = 0; // Router is not reachable time
    ra_hdr->nd_ra_retransmit = 0; // Router is not retransmit timer

    // Set the ICMPv6 options
    char *option_ptr = packet + sizeof(struct nd_router_advert);
    struct nd_opt_prefix_info *prefix_info = (struct nd_opt_prefix_info *)option_ptr;
    prefix_info->nd_opt_pi_type = ND_OPT_PREFIX_INFORMATION;
    prefix_info->nd_opt_pi_len = sizeof(struct nd_opt_prefix_info) / 8; // Length in units of 8 octets
    prefix_info->nd_opt_pi_prefix_len = 64; // Prefix length
    prefix_info->nd_opt_pi_flags_reserved = ND_OPT_PI_FLAG_ONLINK; // Flags
    prefix_info->nd_opt_pi_valid_time = htonl(2592000); // Valid Lifetime: 30 days
    prefix_info->nd_opt_pi_preferred_time = htonl(604800); // Preferred Lifetime: 7 days
    inet_pton(AF_INET6, "2001:db8:1::", &prefix_info->nd_opt_pi_prefix); // Prefix

    // Calculate ICMPv6 checksum
    ra_hdr->nd_ra_hdr.icmp6_cksum = checksum(packet, sizeof(struct nd_router_advert) + sizeof(struct nd_opt_prefix_info));

    // Send the message
    if (sendto(sockfd, packet, sizeof(struct nd_router_advert) + sizeof(struct nd_opt_prefix_info), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Close the socket
    close(sockfd);
}
