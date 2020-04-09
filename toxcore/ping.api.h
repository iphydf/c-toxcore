/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2018 The TokTok team.
 * Copyright © 2013 Tox project.
 * Copyright © 2013 plutooo
 */

#include <stdint.h>

#include "DHT.h"
#include "network.h"

class IP_Port;
class DHT;
class Mono_Time;

/*
 * Buffered pinging using cyclic arrays.
 */
class Ping {

static this new(const Mono_Time *mono_time, DHT *dht);
void kill(void);

/**
 * Add nodes to the to_ping list.
 *
 * All nodes in this list are pinged every TIME_TOPING seconds
 * and are then removed from the list.
 *
 * If the list is full the nodes farthest from our #public_key are replaced.
 * The purpose of this list is to enable quick integration of new nodes into the
 * network while preventing amplification attacks.
 *
 * @return 0 if node was added.
 * @return -1 if node was not added.
 */
int32_t add(const uint8_t *public_key, IP_Port ip_port);
void iterate(void);

int32_t send_request(IP_Port ipp, const uint8_t *public_key);

}
