cmake_minimum_required(VERSION 3.27)
project(dhcpv6_server C)

set(CMAKE_C_STANDARD 11)

add_executable(dhcpv6_server main.c
        messages/router_advertisment/router_advertisement.c
        messages/router_advertisment/router_advertisement.h)
