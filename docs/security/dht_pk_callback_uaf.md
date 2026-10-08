# Use-after-free in `friend_connection.c` `dht_pk_callback`

## Summary

A remotely-triggerable heap use-after-free exists in `dht_pk_callback`
(`toxcore/friend_connection.c`). When a friend connection's DHT public key
changes, the callback invokes `handle_status(..., false, ...)` and then reads the
same `friend_con` pointer again. `handle_status` can run a status callback that
tears a connection down and reallocates the `fr_c->conns` array, leaving
`friend_con` dangling. The subsequent read is a use-after-free.

The bug is **non-local**: the free happens in the conference/group layer,
several calls away from where the stale pointer is read in the friend-connection
layer. It is reachable by a remote peer with no cooperation from the local
client, and it is structurally invisible to the single-process fuzzer.

## Affected code

`toxcore/friend_connection.c`, `dht_pk_callback` (line 433):

```c
static void dht_pk_callback(void *object, int32_t number, const uint8_t *dht_public_key, void *userdata)
{
    Friend_Connections *const fr_c = (Friend_Connections *)object;
    Friend_Conn *const friend_con = get_conn(fr_c, number);   // interior ptr into fr_c->conns
    ...
    change_dht_pk(fr_c, number, dht_public_key);

    if (friend_con->crypt_connection_id != -1) {
        crypto_kill(fr_c->net_crypto, friend_con->crypt_connection_id);
        friend_con->crypt_connection_id = -1;
        handle_status(object, number, false, userdata);       // may realloc fr_c->conns
    }

    onion_set_friend_dht_pubkey(fr_c->onion_c, friend_con->onion_friendnum, ...);  // <-- UAF read
}
```

`get_conn` returns `&fr_c->conns[number]`, i.e. an interior pointer into a
heap array that `realloc_friendconns` (`friend_connection.c:126`) can move or
free. `handle_status` never re-fetches `friend_con` for the caller, so the final
`friend_con->onion_friendnum` read dereferences freed memory.

### The asymmetry that hid it

The sibling dispatchers in the same file, `handle_packet` and
`handle_lossy_packet`, already re-fetch `friend_con = get_conn(fr_c, id)` and
null-check **after each callback**, precisely because a callback can mutate the
connection table. `dht_pk_callback` (and `handle_status`'s own callback loop) do
not. That inconsistency is the defect.

## Root cause: the reallocation chain

`fr_c->conns` is grown by `new_friend_connection` and shrunk by
`wipe_friend_conn` (`friend_connection.c:173`), both via `realloc_friendconns`.
A status callback reaches the shrink path through the conference layer:

```
dht_pk_callback
  -> handle_status(offline)                         friend_connection.c
    -> g_handle_status                               group.c  (registered status callback)
      -> set_conns_status_groups
        -> set_conns_type_connections
          -> check_disconnected        (no ONLINE conference link left -> tear down)
            -> freeze_peer -> delpeer -> remove_connection
              -> kill_friend_connection              friend_connection.c
                -> wipe_friend_conn
                  -> realloc_friendconns             fr_c->conns MOVED/FREED
  back in dht_pk_callback: friend_con->onion_friendnum   use-after-free
```

`kill_friend_connection` only reaches `wipe_friend_conn` (and thus the realloc)
when the connection's `lock_count` is already 0, i.e. a **conference-only**
connection — a peer reached by group introduction (`add_closest_connections` ->
`new_friend_connection`, created with `lock = !fresh = false`), not a direct
Messenger friend.

## Trigger / preconditions

1. The victim is in a conference of at least three peers, holding a
   conference-only (`lock_count == 0`) connection to peer `B` that it reached by
   introduction.
2. That connection is established (`crypt_connection_id != -1`).
3. The conference loses every other ONLINE link (e.g. the introducer leaves), so
   the victim's only remaining conference connection is the one to `B`.
4. `B` announces a **changed** DHT public key to the still-connected victim via
   an `ONION_DATA_DHTPK` announce (`handle_dhtpk_announce` ->
   `dht_pk_callback`). `dht_pk_callback` tears the connection down
   (`handle_status`) and then reads the freed `friend_con`.

A normal peer never announces a changed DHT key to a peer it is already
connected to — `do_friend_announce` skips announcing to online friends — so
step 4 requires a **modified sender**. The victim runs completely unmodified
toxcore. This is why the bug is adversarial and client-independent: it corrupts
memory inside the victim's `tox_iterate` regardless of how well-behaved the
client application is.

## Why fuzzing did not find it

The end-to-end fuzzer drives a single `Tox` instance and never constructs a
multi-peer conference with an introduced peer, nor a modified peer that
re-announces a changed DHT key to an online friend. The unit fuzzers for the
group code do not wire the friend-connection and group layers together. The
required state — a ≥3-peer conference, a conference-only connection, loss of all
other online links, and an adversarial DHT-pk change — is unreachable by those
harnesses. It is, however, catchable by AddressSanitizer once the state is
reached.

## Reproduction

**Fast regression test:** `toxcore/repro_friendconn_status_uaf_test.cc` forms a
two-peer conference, drops the Messenger friendship to leave a conference-only
connection (`lock_count == 0`), and fires a changed-DHT-pk announce through a
test hook. Under `--config=asan` the unmodified code aborts with
`heap-use-after-free ... in dht_pk_callback`; with the fix it passes.

**Live end-to-end:** `other/exploit/friendconn_uaf_attacker.c` drives two real
`Tox` instances over the network against a separate, locally-run buffered-events
client (the `rs-toxcore-c` groupbot) built with `--config=asan`. The attacker
forms a 3-peer conference, the introducer leaves, and the second peer (modified
toxcore) force-announces a changed DHT key to the still-connected victim. ASan
reports the use-after-free inside the victim's `tox_events_iterate` ->
`tox_iterate_internal` -> `do_messenger` -> `networking_poll` ->
`handle_dhtpk_announce` -> `dht_pk_callback`, freed via
`realloc_friendconns` <- `wipe_friend_conn` <- `kill_friend_connection` <-
`remove_connection` <- `delpeer` <- `check_disconnected` <- `g_handle_status`
<- `handle_status`, and originally allocated via
`add_closest_connections` -> `new_friend_connection` (confirming the lock-0
closest-connection path).

## Fix

Re-fetch `friend_con` after `handle_status` and null-check before dereferencing
it, exactly as `handle_packet`/`handle_lossy_packet` already do after their
callbacks (`toxcore/friend_connection.c`, `dht_pk_callback`):

```c
    if (friend_con->crypt_connection_id != -1) {
        crypto_kill(fr_c->net_crypto, friend_con->crypt_connection_id);
        friend_con->crypt_connection_id = -1;
        handle_status(object, number, false, userdata); /* Going offline. */

        /* handle_status() runs the connection's status callbacks, which can tear
         * down connections (e.g. a conference dropping an introduced peer) and
         * realloc fr_c->conns, leaving friend_con dangling. Re-fetch first. */
        friend_con = get_conn(fr_c, number);

        if (friend_con == nullptr) {
            return;
        }
    }

    onion_set_friend_dht_pubkey(fr_c->onion_c, friend_con->onion_friendnum, dht_public_key);
```

With the fix, the fast test passes under ASan and the ASan-built live client
survives the identical sustained attack with no report.

## Test-only scaffolding

The reproducers rely on TEST-ONLY hooks in `onion_client.c`/`onion_client.h`
(`onion_friend_fire_dht_pk_for_test`, `onion_set_fake_dhtpk_for_test`, and the
fake/force-announce branches). These are reproduction aids, not part of the
production fix, which is solely the `dht_pk_callback` re-fetch.
