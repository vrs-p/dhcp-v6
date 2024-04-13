cmake_minimum_required(VERSION 3.27)
project(dhcp-v6 C)

set(CMAKE_C_STANDARD 11)

add_executable(dhcp-v6 main.c
        messages/router_advertisment/router_advertisement.c
        messages/router_advertisment/router_advertisement.h
        server/init.c
        server/init.h
        structures/dhcpv6_struct/dhcpv6_struct.c)
