/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022-2025 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_TOX_NETWORK_H
#define C_TOXCORE_TOXCORE_TOX_NETWORK_H

#include <stdbool.h>
#include <stddef.h>  // size_t

#include "tox_attributes.h"
#include "tox_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Tox_Network_Funcs Tox_Network_Funcs;

typedef struct Tox_Network Tox_Network;

Tox_Network *tox_network_new(const Tox_Network_Funcs *_Nonnull funcs, void *_Nullable user_data, const Tox_Memory *_Nonnull mem);

void tox_network_free(Tox_Network *_Nullable ns);

#define SIZE_IP4 4
#define SIZE_IP6 16
#define SIZE_IP (1 + SIZE_IP6)
#define SIZE_PORT 2
#define SIZE_IPPORT (SIZE_IP + SIZE_PORT)

typedef struct Family {
    uint8_t value;
} Family;

typedef union IP4 {
    uint32_t uint32;
    uint16_t uint16[2];
    uint8_t uint8[4];
} IP4;

typedef union IP6 {
    uint8_t uint8[16];
    uint16_t uint16[8];
    uint32_t uint32[4];
    uint64_t uint64[2];
} IP6;

typedef union IP_Union {
    IP4 v4;
    IP6 v6;
} IP_Union;

typedef struct IP {
    Family family;
    IP_Union ip;
} IP;

typedef struct IP_Port {
    IP ip;
    uint16_t port;
} IP_Port;

typedef tox_bitwise int Socket_Value;
typedef struct Socket {
    Socket_Value value;
} Socket;

int net_socket_to_native(Socket sock);
Socket net_socket_from_native(int sock);

int tox_network_close(const Tox_Network *_Nonnull ns, Socket sock);
Socket tox_network_accept(const Tox_Network *_Nonnull ns, Socket sock);
int tox_network_bind(const Tox_Network *_Nonnull ns, Socket sock, const IP_Port *_Nonnull addr);
int tox_network_listen(const Tox_Network *_Nonnull ns, Socket sock, int backlog);
int tox_network_connect(const Tox_Network *_Nonnull ns, Socket sock, const IP_Port *_Nonnull addr);
int tox_network_recvbuf(const Tox_Network *_Nonnull ns, Socket sock);
int tox_network_recv(const Tox_Network *_Nonnull ns, Socket sock, uint8_t *_Nonnull buf, size_t len);
int tox_network_recvfrom(const Tox_Network *_Nonnull ns, Socket sock, uint8_t *_Nonnull buf, size_t len, IP_Port *_Nonnull addr);
int tox_network_send(const Tox_Network *_Nonnull ns, Socket sock, const uint8_t *_Nonnull buf, size_t len);
int tox_network_sendto(const Tox_Network *_Nonnull ns, Socket sock, const uint8_t *_Nonnull buf, size_t len, const IP_Port *_Nonnull addr);
Socket tox_network_socket(const Tox_Network *_Nonnull ns, int domain, int type, int proto);
int tox_network_socket_nonblock(const Tox_Network *_Nonnull ns, Socket sock, bool nonblock);
int tox_network_getsockopt(const Tox_Network *_Nonnull ns, Socket sock, int level, int optname, void *_Nonnull optval, size_t *_Nonnull optlen);
int tox_network_setsockopt(const Tox_Network *_Nonnull ns, Socket sock, int level, int optname, const void *_Nonnull optval, size_t optlen);
int tox_network_getaddrinfo(const Tox_Network *_Nonnull ns, const Tox_Memory *_Nonnull mem, const char *_Nonnull address, int family, int protocol, IP_Port *_Nonnull *_Nonnull addrs);
int tox_network_freeaddrinfo(const Tox_Network *_Nonnull ns, const Tox_Memory *_Nonnull mem, IP_Port *_Nonnull addrs);

/** @brief Get the last networking error code.
 *
 * Similar to Unix's errno, but cross-platform, as not all platforms use errno
 * to indicate networking errors.
 *
 * Note that different platforms may return different codes for the same error,
 * so you likely shouldn't be checking the value returned by this function
 * unless you know what you are doing, you likely just want to use it in
 * combination with `net_strerror()` to print the error.
 *
 * return platform-dependent network error code, if any.
 */
int net_error(void);

#define NET_STRERROR_SIZE 256

/** @brief Contains a null terminated formatted error message.
 *
 * This struct should not contain more than at most the 2 fields.
 */
typedef struct Net_Strerror {
    char     data[NET_STRERROR_SIZE];
    uint16_t size;
} Net_Strerror;

/** @brief Get a text explanation for the error code from `net_error()`.
 *
 * @param error The error code to get a string for.
 * @param buf The struct to store the error message in (usually on stack).
 *
 * @return pointer to a NULL-terminated string describing the error code.
 */
char *_Nonnull net_strerror(int error, Net_Strerror *_Nonnull buf);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_TOX_NETWORK_H */
