/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Reproducer for a heap-buffer-overflow in dht_save().
 *
 * dht_save() allocates a temporary `clients` array of MAX_SAVED_DHT_NODES
 * entries (DHT.c), where
 *   MAX_SAVED_DHT_NODES == ((DHT_FAKE_FRIEND_NUMBER * MAX_FRIEND_CLIENTS) + LCLIENT_LIST) * 2
 * i.e. it is sized only for the close_clientlist plus the fake-friend client
 * lists. It then writes into that array, starting at index `num ==
 * loaded_num_nodes`, the loaded nodes AND the close list AND the fake-friend
 * lists:
 *
 *     uint32_t num = 0;
 *     if (dht->loaded_num_nodes > 0) { memcpy(clients, loaded_nodes_list, ...); num += loaded_num_nodes; }
 *     for (... close_clientlist ...) clients[num++] = ...;
 *     for (... fake friends ...)     clients[num++] = ...;
 *
 * dht_size() DOES count loaded + close + fake when sizing the on-disk buffer,
 * but the temporary `clients` array does not account for the loaded nodes. So
 * whenever loaded_num_nodes + (populated close assocs) + (populated fake assocs)
 * exceeds MAX_SAVED_DHT_NODES, dht_save() writes past the end of `clients`.
 *
 * loaded_num_nodes is set from the saved state (dht_load) and persists until the
 * node becomes non-LAN connected (dht_connect_after_load clears it). During that
 * window a plain tox_get_savedata() call (which every well-behaved client makes
 * to persist state) corrupts the heap, independent of client quality.
 *
 * This reproduces it deterministically: load MAX_SAVED_DHT_NODES nodes, add a
 * single close-list entry, then dht_save() -> ASan heap-buffer-overflow.
 */
#include "DHT.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <algorithm>
#include <cstring>
#include <vector>

#include "../testing/support/public/simulated_environment.hh"
#include "DHT_test_util.hh"
#include "crypto_core.h"
#include "logger.h"
#include "mono_time.h"
#include "network.h"
#include "network_test_util.hh"
#include "state.h"
#include "test_util.hh"

namespace {

using namespace tox::test;

// On-disk DHT state constants (private to DHT.c; stable wire format).
constexpr uint32_t kDhtStateCookieGlobal = 0x159000d;
constexpr uint16_t kDhtStateCookieType = 0x11ce;
constexpr uint32_t kDhtStateTypeNodes = 4;

// Mirror of DHT.c's MAX_SAVED_DHT_NODES (DHT_FAKE_FRIEND_NUMBER=2,
// MAX_FRIEND_CLIENTS=8, LCLIENT_LIST=LCLIENT_LENGTH(128)*LCLIENT_NODES(8)=1024).
constexpr uint32_t kMaxSavedDhtNodes = ((2 * 8) + 1024) * 2;  // 2080

TEST(ReproDhtSaveOverflow, LoadedNodesPlusCloseListOverflowsTempBuffer)
{
    SimulatedEnvironment env{12345};
    auto c_mem = env.fake_memory().c_memory();
    auto c_rng = env.fake_random().c_random();

    auto node = env.create_node(33445);
    struct Network net_struct = node->c_network;

    Logger *log = logger_new(&c_mem);
    ASSERT_NE(log, nullptr);

    Mono_Time *mono_time = mono_time_new(&c_mem, nullptr, nullptr);
    ASSERT_NE(mono_time, nullptr);
    mono_time_set_current_time_callback(
        mono_time,
        [](void *user_data) -> std::uint64_t {
            auto *clock = static_cast<FakeClock *>(user_data);
            return clock->current_time_ms();
        },
        &env.fake_clock());

    Ptr<Networking_Core> net(new_networking_no_udp(log, &c_mem, &net_struct));
    ASSERT_NE(net, nullptr);
    Ptr<DHT> dht(new_dht(log, &c_mem, &c_rng, &net_struct, mono_time, net.get(), true, true));
    ASSERT_NE(dht, nullptr);

    // Build the "loaded" set. unpack_nodes() (called by dht_load) takes a
    // uint16_t length, so a single NODES section can carry at most ~65535 bytes
    // (~1680 IPv4 nodes). We use 1680 so the whole section unpacks cleanly.
    const uint32_t kLoaded = 1680;
    std::vector<Node_format> nodes(kLoaded);
    uint8_t sk[CRYPTO_SECRET_KEY_SIZE];
    for (uint32_t i = 0; i < kLoaded; ++i) {
        nodes[i] = Node_format{};
        crypto_new_keypair(&c_rng, nodes[i].public_key, sk);  // only pk is used
        nodes[i].ip_port.ip.family = net_family_ipv4();
        nodes[i].ip_port.ip.ip.v4.uint8[0] = 127;
        nodes[i].ip_port.ip.ip.v4.uint8[3] = 1;
        nodes[i].ip_port.port = net_htons(static_cast<uint16_t>(1000 + (i & 0x3fff)));
    }

    std::vector<uint8_t> packed(kLoaded * (CRYPTO_PUBLIC_KEY_SIZE + 32));
    const int packed_len = pack_nodes(log, packed.data(),
                                      static_cast<uint16_t>(packed.size() > 65535 ? 65535 : packed.size()),
                                      nodes.data(), static_cast<uint16_t>(kLoaded));
    ASSERT_GT(packed_len, 0);

    // Wrap in a DHT state blob: [global cookie][section header][packed nodes].
    std::vector<uint8_t> blob(sizeof(uint32_t) + sizeof(uint32_t) * 2 + packed_len);
    uint8_t *p = blob.data();
    // global cookie (little-endian 32)
    p[0] = static_cast<uint8_t>(kDhtStateCookieGlobal & 0xff);
    p[1] = static_cast<uint8_t>((kDhtStateCookieGlobal >> 8) & 0xff);
    p[2] = static_cast<uint8_t>((kDhtStateCookieGlobal >> 16) & 0xff);
    p[3] = static_cast<uint8_t>((kDhtStateCookieGlobal >> 24) & 0xff);
    p += sizeof(uint32_t);
    p = state_write_section_header(p, kDhtStateCookieType, static_cast<uint32_t>(packed_len), kDhtStateTypeNodes);
    std::memcpy(p, packed.data(), packed_len);

    // Load -> sets loaded_num_nodes = kLoaded.
    ASSERT_EQ(dht_load(dht.get(), blob.data(), static_cast<uint32_t>(blob.size())), 0);

    // Fill the close list across distance buckets. A key that matches self in
    // exactly the first `b` bits lands in close-list bucket `b`; vary a tail byte
    // for distinct keys within a bucket. We add both an IPv4 and an IPv6 assoc per
    // key (dht_save counts each non-zero assoc separately), so loaded + close
    // exceeds MAX_SAVED_DHT_NODES and dht_save()'s undersized `clients` array
    // overflows.
    uint8_t self_pk[CRYPTO_PUBLIC_KEY_SIZE];
    std::memcpy(self_pk, dht_get_self_public_key(dht.get()), sizeof(self_pk));
    for (uint32_t b = 0; b < 64; ++b) {           // buckets 0..63
        for (uint32_t v = 0; v < 8; ++v) {        // up to 8 keys per bucket
            uint8_t pk[CRYPTO_PUBLIC_KEY_SIZE];
            std::memcpy(pk, self_pk, sizeof(pk));
            pk[b / 8] ^= static_cast<uint8_t>(0x80u >> (b % 8));  // flip bit b -> shared prefix == b
            pk[CRYPTO_PUBLIC_KEY_SIZE - 1] = static_cast<uint8_t>(v);  // distinct within bucket

            IP_Port ip4 = {0};
            ip4.ip.family = net_family_ipv4();
            ip4.ip.ip.v4.uint8[0] = 127;
            ip4.ip.ip.v4.uint8[3] = 2;
            ip4.port = net_htons(static_cast<uint16_t>(2000 + b * 8 + v));
            addto_lists(dht.get(), &ip4, pk);

            IP_Port ip6 = {0};
            ip6.ip.family = net_family_ipv6();
            ip6.ip.ip.v6.uint8[0] = 0xfd;
            ip6.ip.ip.v6.uint8[15] = 1;
            ip6.port = net_htons(static_cast<uint16_t>(3000 + b * 8 + v));
            addto_lists(dht.get(), &ip6, pk);
        }
    }

    // dht_size() accounts for loaded+close+fake, but dht_save()'s internal
    // `clients` array only accounts for close+fake -> heap overflow here.
    std::vector<uint8_t> out(dht_size(dht.get()));
    dht_save(dht.get(), out.data());

    // If we get here without ASan firing, the overflow did not reproduce.
    fprintf(stderr, "dht_save completed without ASan firing\n");

    mono_time_free(&c_mem, mono_time);
    logger_kill(log);
}

}  // namespace
