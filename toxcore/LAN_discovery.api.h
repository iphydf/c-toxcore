/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2018 The TokTok team.
 * Copyright © 2013 Tox project.
 */

#include "DHT.h"

class DHT;
class IP;

/*
 * LAN discovery implementation.
 */
namespace lan_discovery {

/**
 * Interval in seconds between LAN discovery packet sending.
 */
#define INTERVAL 10

/**
 * Send a LAN discovery packet to the broadcast address with port #port.
 */
int32_t send(uint16_t port, DHT *dht);

/**
 * Sets up packet handlers.
 */
void init(DHT *dht);

/**
 * Clear packet handlers.
 */
void kill(DHT *dht);

}

/**
 * Is IP a local ip or not.
 */
bool ip_is_local(IP ip);

/**
 * Checks if a given IP isn't routable.
 *
 * @return true if ip is a LAN ip, false if it is not.
 */
bool ip_is_lan(IP ip);
