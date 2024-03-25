/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022-2025 The TokTok team.
 */
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif /* __APPLE__ */

// For Solaris.
#ifdef __sun
#define __EXTENSIONS__ 1
#endif /* __sun */

// For Linux (and some BSDs).
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif /* _XOPEN_SOURCE */

#if defined(_WIN32) && _WIN32_WINNT >= _WIN32_WINNT_WINXP
#undef _WIN32_WINNT
#define _WIN32_WINNT  0x501
#endif /* _WIN32 */

#if !defined(OS_WIN32) && (defined(_WIN32) || defined(__WIN32__) || defined(WIN32))
#define OS_WIN32
#endif /* OS_WIN32 */

#if defined(OS_WIN32) && !defined(WINVER)
// Windows XP
#define WINVER 0x0501
#endif /* OS_WIN32 */

#include "os_network.h"

#ifdef OS_WIN32 // Put win32 includes here
// The mingw32/64 Windows library warns about including winsock2.h after
// windows.h even though with the above it's a valid thing to do. So, to make
// mingw32 headers happy, we include winsock2.h first.
#include <winsock2.h>
// Comment line here to avoid reordering by source code formatters.
#include <windows.h>
#include <ws2tcpip.h>
#endif /* OS_WIN32 */

#if !defined(OS_WIN32)
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __sun
#include <stropts.h>
#include <sys/filio.h>
#endif /* __sun */

#else
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif /* IPV6_V6ONLY */
#endif /* OS_WIN32 */

#include <assert.h>
#include <string.h>  // memcpy

#include "attributes.h"
#include "ccompat.h"
#include "mem.h"
#include "os_network_impl.h"
#include "tox_memory.h"
#include "tox_network.h"
#include "tox_network_impl.h"  // IWYU pragma: keep

// Disable MSG_NOSIGNAL on systems not supporting it, e.g. Windows, FreeBSD
#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif /* MSG_NOSIGNAL */

#ifndef OS_WIN32
#include <errno.h>
#endif /* !OS_WIN32 */

/** Redefinitions of variables for safe transfer over wire. */
#define TOX_AF_INET 2
#define TOX_AF_INET6 10

static const Family family_ipv4 = {TOX_AF_INET};
static const Family family_ipv6 = {TOX_AF_INET6};

static Family net_family_ipv4(void)
{
    return family_ipv4;
}

static Family net_family_ipv6(void)
{
    return family_ipv6;
}

static bool net_family_is_ipv4(Family family)
{
    return family.value == family_ipv4.value;
}

static bool net_family_is_ipv6(Family family)
{
    return family.value == family_ipv6.value;
}

static void get_ip4(IP4 *_Nonnull result, const struct in_addr *_Nonnull addr)
{
    static_assert(sizeof(result->uint32) == sizeof(addr->s_addr),
                  "Tox and operating system don't agree on size of IPv4 addresses");
    result->uint32 = addr->s_addr;
}

static void get_ip6(IP6 *_Nonnull result, const struct in6_addr *_Nonnull addr)
{
    static_assert(sizeof(result->uint8) == sizeof(addr->s6_addr),
                  "Tox and operating system don't agree on size of IPv6 addresses");
    memcpy(result->uint8, addr->s6_addr, sizeof(result->uint8));
}

static void fill_addr4(const IP4 *_Nonnull ip, struct in_addr *_Nonnull addr)
{
    addr->s_addr = ip->uint32;
}

static void fill_addr6(const IP6 *_Nonnull ip, struct in6_addr *_Nonnull addr)
{
    memcpy(addr->s6_addr, ip->uint8, sizeof(ip->uint8));
}

static int os_close(void *_Nonnull self, Socket sock)
{
#if defined(OS_WIN32)
    return closesocket(net_socket_to_native(sock));
#else  // !OS_WIN32
    return close(net_socket_to_native(sock));
#endif /* OS_WIN32 */
}

typedef struct Network_Addr {
    struct sockaddr_storage addr;
    size_t size;
} Network_Addr;

static void ip_port_to_network_addr(const IP_Port *ip_port, Network_Addr *addr)
{
    addr->size = 0;
    if (net_family_is_ipv4(ip_port->ip.family)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr->addr;
        addr->size = sizeof(struct sockaddr_in);
        addr4->sin_family = AF_INET;
        fill_addr4(&ip_port->ip.ip.v4, &addr4->sin_addr);
        addr4->sin_port = ip_port->port;
    } else if (net_family_is_ipv6(ip_port->ip.family)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr->addr;
        addr->size = sizeof(struct sockaddr_in6);
        addr6->sin6_family = AF_INET6;
        fill_addr6(&ip_port->ip.ip.v6, &addr6->sin6_addr);
        addr6->sin6_port = ip_port->port;
        addr6->sin6_flowinfo = 0;
        addr6->sin6_scope_id = 0;
    }
}

static bool network_addr_to_ip_port(const Network_Addr *addr, IP_Port *ip_port)
{
    if (addr->addr.ss_family == AF_INET) {
        const struct sockaddr_in *addr_in = (const struct sockaddr_in *)&addr->addr;
        ip_port->ip.family = net_family_ipv4();
        get_ip4(&ip_port->ip.ip.v4, &addr_in->sin_addr);
        ip_port->port = addr_in->sin_port;
        return true;
    } else if (addr->addr.ss_family == AF_INET6) {
        const struct sockaddr_in6 *addr_in6 = (const struct sockaddr_in6 *)&addr->addr;
        ip_port->ip.family = net_family_ipv6();
        get_ip6(&ip_port->ip.ip.v6, &addr_in6->sin6_addr);
        ip_port->port = addr_in6->sin6_port;
        return true;
    }
    return false;
}

static Socket os_accept(void *_Nonnull self, Socket sock)
{
    return net_socket_from_native(accept(net_socket_to_native(sock), nullptr, nullptr));
}

static int os_bind(void *_Nonnull self, Socket sock, const IP_Port *_Nonnull addr)
{
    Network_Addr naddr;
    ip_port_to_network_addr(addr, &naddr);
    if (naddr.size == 0) {
        return -1;
    }
    return bind(net_socket_to_native(sock), (const struct sockaddr *)&naddr.addr, naddr.size);
}

static int os_listen(void *_Nonnull self, Socket sock, int backlog)
{
    return listen(net_socket_to_native(sock), backlog);
}

static int os_connect(void *_Nonnull self, Socket sock, const IP_Port *_Nonnull addr)
{
    Network_Addr naddr;
    ip_port_to_network_addr(addr, &naddr);
    if (naddr.size == 0) {
        return -1;
    }
    return connect(net_socket_to_native(sock), (const struct sockaddr *)&naddr.addr, naddr.size);
}

static int os_recvbuf(void *_Nonnull self, Socket sock)
{
#ifdef OS_WIN32
    u_long count = 0;
    ioctlsocket(net_socket_to_native(sock), FIONREAD, &count);
#else
    int count = 0;
    ioctl(net_socket_to_native(sock), FIONREAD, &count);
#endif /* OS_WIN32 */

    return count;
}

static int os_recv(void *_Nonnull self, Socket sock, uint8_t *_Nonnull buf, size_t len)
{
    return recv(net_socket_to_native(sock), (char *)buf, len, MSG_NOSIGNAL);
}

static int os_send(void *_Nonnull self, Socket sock, const uint8_t *_Nonnull buf, size_t len)
{
    return send(net_socket_to_native(sock), (const char *)buf, len, MSG_NOSIGNAL);
}

static int os_sendto(void *_Nonnull self, Socket sock, const uint8_t *_Nonnull buf, size_t len, const IP_Port *_Nonnull addr)
{
    Network_Addr naddr;
    ip_port_to_network_addr(addr, &naddr);
    if (naddr.size == 0) {
        return -1;
    }
    return sendto(net_socket_to_native(sock), (const char *)buf, len, 0, (const struct sockaddr *)&naddr.addr, naddr.size);
}

static int os_recvfrom(void *_Nonnull self, Socket sock, uint8_t *_Nonnull buf, size_t len, IP_Port *_Nonnull addr)
{
    Network_Addr naddr;
    naddr.size = sizeof(naddr.addr);
    const int ret = recvfrom(net_socket_to_native(sock), (char *)buf, len, 0, (struct sockaddr *)&naddr.addr, (socklen_t *)&naddr.size);
    if (ret >= 0) {
        if (!network_addr_to_ip_port(&naddr, addr)) {
            // Ignore packets from unknown families
            // But we must return -1 so the caller knows we dropped it?
            // network.c implementation returned -1 here.
            return -1;
        }
    }
    return ret;
}

static Socket os_socket(void *_Nonnull self, int domain, int type, int proto)
{
    return net_socket_from_native(socket(domain, type, proto));
}

static int os_socket_nonblock(void *_Nonnull self, Socket sock, bool nonblock)
{
#ifdef OS_WIN32
    u_long mode = nonblock ? 1 : 0;
    return ioctlsocket(net_socket_to_native(sock), FIONBIO, &mode);
#else
    return fcntl(net_socket_to_native(sock), F_SETFL, O_NONBLOCK, nonblock ? 1 : 0);
#endif /* OS_WIN32 */
}

static int os_getsockopt(void *_Nonnull self, Socket sock, int level, int optname, void *_Nonnull optval, size_t *_Nonnull optlen)
{
    char *optval_bytes = (char *)optval;
    socklen_t len = *optlen;
    const int ret = getsockopt(net_socket_to_native(sock), level, optname, optval_bytes, &len);
    *optlen = len;
    return ret;
}

static int os_setsockopt(void *_Nonnull self, Socket sock, int level, int optname, const void *_Nonnull optval, size_t optlen)
{
#ifdef EMSCRIPTEN
    return 0;
#else
    const char *optval_bytes = (const char *)optval;
    return setsockopt(net_socket_to_native(sock), level, optname, optval_bytes, optlen);
#endif /* EMSCRIPTEN */
}

// sets and fills an array of addrs for address
// returns the number of entries in addrs
static int os_getaddrinfo(void *_Nonnull obj, const Tox_Memory *_Nonnull mem, const char *_Nonnull address, int family, int sock_type, IP_Port *_Nullable *_Nonnull addrs)
{
    assert(addrs != nullptr);

    struct addrinfo hints = {0};
    hints.ai_family = family;



    // different platforms favour a different field
    // hints.ai_socktype = SOCK_DGRAM; // type of socket Tox uses.
    hints.ai_socktype = sock_type;
    // hints.ai_protocol = protocol;

    struct addrinfo *infos = nullptr;

    const int rc = getaddrinfo(address, nullptr, &hints, &infos);

    // Lookup failed.
    if (rc != 0) {
        // TODO(Green-Sky): log error
        return 0;
    }

    const int32_t max_count = INT32_MAX / sizeof(IP_Port);

    // we count number of "valid" results
    int result = 0;
    for (struct addrinfo *walker = infos; walker != nullptr && result < max_count; walker = walker->ai_next) {
        if (walker->ai_family == family || family == AF_UNSPEC) {
            ++result;
        }

        // do we need to check socktype/protocol?
    }

    assert(max_count >= result);

    IP_Port *tmp_addrs = (IP_Port *)tox_memory_alloc(mem, result * sizeof(IP_Port));
    if (tmp_addrs == nullptr) {
        freeaddrinfo(infos);
        return 0;
    }

    // now we fill in
    int i = 0;
    for (struct addrinfo *walker = infos; walker != nullptr; walker = walker->ai_next) {
        if (walker->ai_family == family || family == AF_UNSPEC) {
            Network_Addr naddr;
            naddr.size = walker->ai_addrlen;
            memcpy(&naddr.addr, walker->ai_addr, walker->ai_addrlen);

            if (network_addr_to_ip_port(&naddr, &tmp_addrs[i])) {
                ++i;
            }
        }
    }

    // Correct the count if conversion failed for some reason
    result = i;

    freeaddrinfo(infos);

    *addrs = tmp_addrs;

    // number of entries in addrs
    return result;
}

static int os_freeaddrinfo(void *_Nonnull obj, const Tox_Memory *_Nonnull mem, IP_Port *_Nonnull addrs)
{
    if (addrs == nullptr) {
        return 0;
    }

    tox_memory_dealloc(mem, addrs);

    return 0;
}

static const Tox_Network_Funcs os_network_funcs = {
    os_close,
    os_accept,
    os_bind,
    os_listen,
    os_connect,
    os_recvbuf,
    os_recv,
    os_recvfrom,
    os_send,
    os_sendto,
    os_socket,
    os_socket_nonblock,
    os_getsockopt,
    os_setsockopt,
    os_getaddrinfo,
    os_freeaddrinfo,
};
const Tox_Network os_network_obj = {&os_network_funcs};

#if 0
/* TODO(iphydf): Call this from functions that use `os_network()`. */
void os_network_deinit(const Network *ns)
{
#ifdef OS_WIN32
    WSACleanup();
#endif /* OS_WIN32 */
}
#endif /* 0 */

const Tox_Network *os_network(void)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    if ((true)) {
        return nullptr;
    }
#endif /* FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION */
#ifdef OS_WIN32
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
        return nullptr;
    }
#endif /* OS_WIN32 */
    return &os_network_obj;
}

int net_error(void)
{
#ifdef OS_WIN32
    return WSAGetLastError();
#else
    return errno;
#endif /* OS_WIN32 */
}

char *net_strerror(int error, Net_Strerror *buf)
{
#ifdef OS_WIN32
    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        buf->data, NET_STRERROR_SIZE, NULL)) {
        snprintf(buf->data, NET_STRERROR_SIZE, "Unknown error %d", error);
    }
#else
    strerror_r(error, buf->data, NET_STRERROR_SIZE);
#endif /* OS_WIN32 */
    buf->data[NET_STRERROR_SIZE - 1] = '\0';
    buf->size = strlen(buf->data);
    return buf->data;
}
