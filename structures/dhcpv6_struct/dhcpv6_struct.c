//
// Created by student on 13.4.2024.
//

#include <stdint-gcc.h>

/**
 * struct msg_hdr - DHCPv6 client/server message header
 * @type:		DHCP message type
 * @xid:		Transaction ID for message exchange
 */
struct msg_hdr {
    uint32_t type:8;
#define TYPE_SOLICIT			1
#define TYPE_ADVERTISE			2
#define TYPE_REQUEST			3
#define TYPE_CONFIRM			4
#define TYPE_RENEW			5
#define TYPE_REBIND			6
#define TYPE_REPLY			7
#define TYPE_RELEASE			8
#define TYPE_DECLINE			9
#define TYPE_INFORMATION_REQUEST	11

    uint32_t xid:24;
} __attribute__((__packed__));

/**
 * struct opt_hdr - DHCPv6 option header
 * @t:		Option type
 * @l:		Option length, network order
 */
struct opt_hdr {
    uint16_t t;
    uint16_t l;
} __attribute__((packed));

/**
 * struct opt_client_id - DHCPv6 Client Identifier option
 * @hdr:		Option header
 * @duid:		Client DUID, up to 128 bytes (cf. RFC 8415, 11.1.)
 */
struct opt_client_id {
    struct opt_hdr hdr;
    uint8_t duid[];
} __attribute__((packed));

/**
 * struct opt_server_id - DHCPv6 Server Identifier option
 * @hdr:        Option header
 * @duid:       Server DUID
 */
struct opt_server_id {
    struct opt_hdr hdr;
    uint8_t duid[128];
} __attribute__((packed));