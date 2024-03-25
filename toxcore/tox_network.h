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

/**
 * @brief Wrapper for sockaddr_storage and size.
 */
typedef struct Network_Addr Network_Addr;

typedef tox_bitwise int Socket_Value;
typedef struct Socket {
    Socket_Value value;
} Socket;

int net_socket_to_native(Socket sock);
Socket net_socket_from_native(int sock);

int tox_network_close(const Tox_Network *_Nonnull ns, Socket sock);
Socket tox_network_accept(const Tox_Network *_Nonnull ns, Socket sock);
int tox_network_bind(const Tox_Network *_Nonnull ns, Socket sock, const Network_Addr *_Nonnull addr);
int tox_network_listen(const Tox_Network *_Nonnull ns, Socket sock, int backlog);
int tox_network_connect(const Tox_Network *_Nonnull ns, Socket sock, const Network_Addr *_Nonnull addr);
int tox_network_recvbuf(const Tox_Network *_Nonnull ns, Socket sock);
int tox_network_recv(const Tox_Network *_Nonnull ns, Socket sock, uint8_t *_Nonnull buf, size_t len);
int tox_network_recvfrom(const Tox_Network *_Nonnull ns, Socket sock, uint8_t *_Nonnull buf, size_t len, Network_Addr *_Nonnull addr);
int tox_network_send(const Tox_Network *_Nonnull ns, Socket sock, const uint8_t *_Nonnull buf, size_t len);
int tox_network_sendto(const Tox_Network *_Nonnull ns, Socket sock, const uint8_t *_Nonnull buf, size_t len, const Network_Addr *_Nonnull addr);
Socket tox_network_socket(const Tox_Network *_Nonnull ns, int domain, int type, int proto);
int tox_network_socket_nonblock(const Tox_Network *_Nonnull ns, Socket sock, bool nonblock);
int tox_network_getsockopt(const Tox_Network *_Nonnull ns, Socket sock, int level, int optname, void *_Nonnull optval, size_t *_Nonnull optlen);
int tox_network_setsockopt(const Tox_Network *_Nonnull ns, Socket sock, int level, int optname, const void *_Nonnull optval, size_t optlen);
int tox_network_getaddrinfo(const Tox_Network *_Nonnull ns, const Tox_Memory *_Nonnull mem, const char *_Nonnull address, int family, int protocol, Network_Addr *_Nonnull *_Nonnull addrs);
int tox_network_freeaddrinfo(const Tox_Network *_Nonnull ns, const Tox_Memory *_Nonnull mem, Network_Addr *_Nonnull addrs);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_TOX_NETWORK_H */
