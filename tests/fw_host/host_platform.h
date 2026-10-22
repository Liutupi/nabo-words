/* Host-only socket and pacing adapters. The production firmware is unchanged. */
#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
typedef SOCKET host_socket_t;
typedef int host_socklen_t;
#define HOST_INVALID_SOCKET INVALID_SOCKET
static int host_network_init(void)
{
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data);
}
static int host_nonblock(host_socket_t socket)
{
    u_long one = 1;
    return ioctlsocket(socket, FIONBIO, &one);
}
static void host_close(host_socket_t socket) { closesocket(socket); }
static void host_pause(void) { Sleep(33); }
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int host_socket_t;
typedef socklen_t host_socklen_t;
#define HOST_INVALID_SOCKET (-1)
static int host_network_init(void) { return 0; }
static int host_nonblock(host_socket_t socket) { return fcntl(socket, F_SETFL, O_NONBLOCK); }
static void host_close(host_socket_t socket) { close(socket); }
static void host_pause(void) { usleep(33000); }
#endif
