#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <netinet/ip6.h>

#include <sys/ioctl.h>

#define MULTICAST_ADDR "ff02::1"
#define PORT 12345
#define INTERFACE_NAME "enp0s8"


#define BUF_SIZE 1024

// Function to calculate the checksum for ICMPv6 message
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

void send_RA_response()
{
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

int main() {
    int sockfd;
    char buffer[BUF_SIZE];
    struct sockaddr_in6 dest, client;
    struct icmp6_hdr *icmp6_hdr;
    struct ipv6_mreq mreq;
    socklen_t client_len;
    int bytes_sent, bytes_received;
    const char *interface_name = "enp0s8"; // Change this to your interface name
    int if_index = if_nametoindex(interface_name);


    // Create a raw socket
    sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // BIND on all addresses
    memset(&dest, 0, sizeof(dest));
    dest.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::", &dest.sin6_addr);
    if (bind (sockfd,(struct sockaddr *) &dest,sizeof (dest)) < 0)
        exit(EXIT_FAILURE);

    // Join the link-local all nodes multicast group
    inet_pton (AF_INET6,"FF02::16",&mreq.ipv6mr_multiaddr);
    mreq.ipv6mr_interface = 0;
    if (setsockopt (sockfd,IPPROTO_IPV6,IPV6_ADD_MEMBERSHIP,(char *) &mreq,sizeof (mreq)) < 0)
        exit(EXIT_FAILURE);

//    // Prepare the ICMPv6 Router Advertisement message
    memset(buffer, 0, BUF_SIZE);
    icmp6_hdr = (struct icmp6_hdr *) buffer;
    icmp6_hdr->icmp6_type = ND_ROUTER_ADVERT;
    icmp6_hdr->icmp6_code = 0;
    icmp6_hdr->icmp6_cksum = checksum(buffer, sizeof(struct icmp6_hdr));

    // Set the M-flag to 1 (Managed Address Configuration Flag)
    buffer[sizeof(struct icmp6_hdr)] = 0x80; // 0x80 is the M-flag bit set to 1

    // Main loop to receive and respond to Router Solicitation messages
    int response_was_sent = 0;
    while (!response_was_sent) {
        // Receive Router Solicitation message from client
        bytes_received = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&client, &client_len);
        if (bytes_received == -1) {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        if (bytes_received > 0)
        {
            printf("Router Solicitation received from: %s\n", inet_ntop(AF_INET6, &client.sin6_addr, buffer, INET6_ADDRSTRLEN));
            // Send Router Advertisement message in response
            send_RA_response();
            printf("Router Advertisement sent\n");
            response_was_sent = 1;
//            bytes_sent = sendto(sockfd, buffer, sizeof(struct icmp6_hdr), 0, (struct sockaddr *)&dest, sizeof(dest));
//            if (bytes_sent == -1) {
//                perror("sendto");
//                close(sockfd);
//                exit(EXIT_FAILURE);
//            }
//
//            if (bytes_sent > 0)
//            {
//                printf("Router Advertisement sent to: %s\n", inet_ntop(AF_INET6, &dest.sin6_addr, buffer, INET6_ADDRSTRLEN));
//                response_was_sent = 1;
//            }
        }
    }

    close(sockfd);
    return 0;
}
