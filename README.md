# dhcp-v6



## Usefull links
- Captured DHCPv6 reqeust in wireshark [link](https://www.cloudshark.org/captures/d5adb5c21c64)
- How stateful DHCPv6 works [link](https://www.networkacademy.io/ccna/ipv6/stateful-dhcpv6)
- Structures for DHCPv6 in C language [link](https://passt.top/passt/tree/dhcpv6.c)

## What needs to be done

- Create every DHCPv6 stateful message
  - [x] Router advertisment
  - [ ] Router solicit
  - [x] Router advertise
  - [ ] Router request
  - [x] Router reply
- How it should be done from my point of view is that you will listen on every request, after the message is received you will reply with specific response
