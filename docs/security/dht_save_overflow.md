# Heap buffer overflow in `DHT.c` `dht_save`

## Summary

`dht_save` (`toxcore/DHT.c`) writes the node list it is about to serialise into a
temporary `clients` array, but sizes that array for only part of what it writes.
The array is allocated with `MAX_SAVED_DHT_NODES` entries, which covers the close
list plus the fake-friend client lists, but `dht_save` then writes the *loaded*
nodes (`dht->loaded_num_nodes`) **before** the close and fake lists. Whenever
`loaded_num_nodes` is non-zero and the close/fake lists are populated enough that

    loaded_num_nodes + (populated close assocs) + (populated fake assocs) > MAX_SAVED_DHT_NODES

the writes run past the end of `clients` — a heap buffer overflow.

The bug is a **composite**: the sister function `dht_size`, which sizes the
on-disk output buffer, *does* count the loaded nodes plus the close and fake
lists, so the output buffer is large enough. Only the temporary `clients` array
disagrees with `dht_size` about how many nodes `dht_save` can emit. Each function
looks correct in isolation; the defect is the disagreement between them.

It is present on `master` as well as this branch (the file is unchanged on the
branch). See "Reachability" below: despite first appearances this is a latent
under-allocation, not a network-triggerable crash, because `loaded_num_nodes` is
capped (~1680) and the close list cannot realistically be filled past the buffer
by real nodes. The fix is hardening.

## Affected code

`toxcore/DHT.c`:

```c
#define MAX_SAVED_DHT_NODES (((DHT_FAKE_FRIEND_NUMBER * MAX_FRIEND_CLIENTS) + LCLIENT_LIST) * 2)

void dht_save(const DHT *dht, uint8_t *data)
{
    ...
    Node_format *clients = (Node_format *)mem_valloc(dht->mem, MAX_SAVED_DHT_NODES, sizeof(Node_format));
    ...
    uint32_t num = 0;

    if (dht->loaded_num_nodes > 0) {
        memcpy(clients, dht->loaded_nodes_list, sizeof(Node_format) * dht->loaded_num_nodes);
        num += dht->loaded_num_nodes;                 // num can start as high as ~1680
    }

    for (uint32_t i = 0; i < LCLIENT_LIST; ++i) {     // writes up to 2*LCLIENT_LIST more
        if (dht->close_clientlist[i].assoc4.timestamp != 0) { clients[num] = ...; ++num; }   // OOB
        if (dht->close_clientlist[i].assoc6.timestamp != 0) { clients[num] = ...; ++num; }   // OOB
    }

    for (... DHT_FAKE_FRIEND_NUMBER fake friends ...) { ... clients[num] = ...; ++num; }      // OOB
    ...
}
```

With `DHT_FAKE_FRIEND_NUMBER = 2`, `MAX_FRIEND_CLIENTS = 8`,
`LCLIENT_LIST = 1024`, `MAX_SAVED_DHT_NODES = 2080`. The close loop alone can
write `2 * 1024 = 2048` and the fake loop `2 * 16 = 32` — together exactly the
2080 entries the buffer is sized for — while `num` already started at
`loaded_num_nodes`.

`loaded_num_nodes` is set from the saved state by `dht_load` (capped by
`unpack_nodes`, whose `length` argument is `uint16_t`, at ~1680 nodes) and is
cleared only once the node becomes non-LAN connected
(`dht_connect_after_load`). During that startup window a save corrupts the heap.

## Reachability — latent / hardening, NOT remotely triggerable

This is a real buffer under-allocation but, on closer analysis, it is **not**
reachable from the network, so it is a hardening fix rather than an exploitable
crash:

- `loaded_num_nodes` is capped at ~1680: `dht_load` → `unpack_nodes` takes a
  `uint16_t` length, so a single NODES section is at most 65535 bytes ≈ 1680
  IPv4 nodes. `loaded` alone cannot reach the 2080-entry buffer.
- To overflow, `close + fake` must therefore add > ~400 entries.
  `close_clientlist` is populated only via `addto_lists` (`DHT.c:1505`), which
  fires only on a validated sendnodes *response* (getnodes/sendnodes handshake,
  ping-id matched, decrypted with the responder's shared key). So every close
  entry is a real node that completed the handshake, i.e. a node with a valid
  keypair — and its keypair fixes its distance bucket. Random keypairs cluster
  in the low buckets; occupying high buckets needs grinding a keypair that
  shares ~20+ prefix bits with the target (astronomically expensive). A node
  cannot be made to occupy arbitrary buckets on demand.
- Consequently a realistic close list reaches only a few hundred assocs, and
  `loaded (≤1680) + close + fake` cannot realistically exceed 2080. The one
  condition that keeps `loaded` alive (never non-LAN connected) is also
  anti-correlated with a well-populated close list.

The reproducer below injects close entries through the internal `addto_lists`
directly (no handshake, so keys can be placed in any bucket); that is an
in-process state, not one an attacker can drive over the network. It documents
the under-allocation and verifies the fix; it is not a demonstration of a
remote crash.

## Fix

Size the temporary array for everything `dht_save` writes — the loaded nodes in
addition to `MAX_SAVED_DHT_NODES` — matching what `dht_size` already counts:

```c
const uint32_t max_clients = MAX_SAVED_DHT_NODES + dht->loaded_num_nodes;
Node_format *clients = (Node_format *)mem_valloc(dht->mem, max_clients, sizeof(Node_format));
```

## Reproducer

`toxcore/repro_dht_save_overflow_test.cc` loads 1680 nodes (sets
`loaded_num_nodes`), fills the close list across distance buckets (both IPv4 and
IPv6 assocs), then calls `dht_save` into a `dht_size()`-sized buffer. Before the
fix it fails under ASan with a heap-buffer-overflow at `DHT.c` `dht_save` (the
close-list `memcpy`); after the fix it completes cleanly.
