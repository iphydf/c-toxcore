# Use-after-free in `group_chats.c` `handle_gc_packet_fragment`

## Summary

A remotely-triggerable heap use-after-free exists in `handle_gc_packet_fragment`
(`toxcore/group_chats.c`). The function holds a `GC_Connection *gconn` pointer —
an interior pointer into the reallocatable `chat->group[]` array — across a call
to `gcc_handle_packet_fragment`, then dereferences it again. When the fragment
sequence completes, `gcc_handle_packet_fragment` reassembles the packet and runs
its handler; if that handler adds a peer, `chat->group[]` is reallocated and
`gconn` is left dangling. The subsequent dereferences (`gc_send_message_ack` and
the `last_received_packet_time` write) are a use-after-free.

The bug is **non-local**: the free happens deep inside the group sync/peer-add
logic, several calls away from where the stale pointer is read in the fragment
dispatcher. It is reachable by a confirmed group peer with no cooperation from
the local client, and it is structurally invisible to the fuzzers.

It is the NGC sibling of the `dht_pk_callback` use-after-free
([`dht_pk_callback_uaf.md`](dht_pk_callback_uaf.md)): the same
"interior pointer into a reallocatable array held across a callback that can
realloc it" defect, in a different layer.

## Affected code

`toxcore/group_chats.c`, `handle_gc_packet_fragment`:

```c
static bool handle_gc_packet_fragment(..., uint32_t peer_number, GC_Connection *gconn,
                                      const uint8_t *data, uint16_t length, uint8_t packet_type,
                                      uint64_t message_id, void *userdata)
{
    ...
    const int frag_ret = gcc_handle_packet_fragment(c, chat, peer_number, gconn, data, length,
                                                     packet_type, message_id, userdata);  // may realloc chat->group
    if (frag_ret == -1) {
        return false;
    }

    if (frag_ret == 0) {
        gc_send_message_ack(chat, gconn, message_id, GR_ACK_RECV);   // <-- UAF read
    }

    gconn->last_received_packet_time = mono_time_get(chat->mono_time);  // <-- UAF write
    return true;
}
```

`get_gc_connection(chat, n)` returns `&chat->group[n].gconn`, i.e. an interior
pointer into a heap array that `peer_add` (`group_chats.c`, via `mem_vrealloc`)
can move. The `gconn` passed into `handle_gc_packet_fragment` points into that
array; after `gcc_handle_packet_fragment` adds a peer, the array moves and
`gconn` dangles.

### The asymmetry that hid it

The re-fetch idiom for exactly this hazard is present everywhere around this
code — but one frame too deep:

- `gcc_handle_packet_fragment` (`group_connection.c`) itself re-fetches **its
  own** local `gconn` after running the reassembled packet's handler, with the
  comment *"peer number can change from peer add operations in packet
  handlers"*. But it returns only an `int`, so the caller's `gconn` is not
  updated.
- `handle_gc_lossless_packet` re-fetches `gconn` (via `sender_pk`) after **its**
  `handle_gc_lossless_helper` call for the same reason.
- `process_recv_array_entry` (`group_connection.c`) re-fetches `gconn` after its
  `handle_gc_lossless_helper` call.

`handle_gc_packet_fragment`, sitting between the caller that re-fetches and the
callee that re-fetches internally, does not. That inconsistency is the defect.

## Root cause: the reallocation chain

```
handle_gc_packet_fragment            holds gconn = &chat->group[n].gconn
  -> gcc_handle_packet_fragment      group_connection.c
       (final, empty fragment) reassembles the sequence and runs:
     -> handle_gc_lossless_helper
       -> handle_gc_sync_response
         -> unpack_gc_sync_announce
           -> peer_add
             -> mem_vrealloc(chat->group)      chat->group MOVED/FREED
  back in handle_gc_packet_fragment:
    gc_send_message_ack(chat, gconn, ...)       use-after-free (read)
    gconn->last_received_packet_time = ...       use-after-free (write)
```

`peer_add` grows `chat->group` whenever a previously-unknown peer is announced.
NGC peer *deletion* is deferred to a cleanup pass (`do_peer_delete`), so the
in-handler hazard is specifically the **grow** (realloc/move), reached here
through the sync-response announce path.

## Trigger / preconditions

1. The attacker is a confirmed peer in an NGC group with the victim (a normal
   join is sufficient; the lossless channel must be established, i.e.
   `gconn->handshaked`).
2. The attacker sends a **fragmented** lossless packet sequence (`GP_FRAGMENT`
   chunks followed by the terminating empty fragment) whose reassembled payload
   is a `GP_SYNC_RESPONSE` carrying a valid announce
   (`gca_is_valid_announce`) for a peer the victim does not yet know.
3. On the terminating fragment, the victim reassembles the packet and runs
   `handle_gc_sync_response -> unpack_gc_sync_announce -> peer_add`, which
   reallocates `chat->group`. `handle_gc_packet_fragment` then dereferences the
   dangling `gconn`.

A normal peer never triggers this: legitimate sync responses are sent **one
announce per packet** (`sync_response_send_peers`), each well under
`MAX_GC_PACKET_CHUNK_SIZE` (500 bytes), so a sync response is never fragmented.
Delivering a *fragmented* sync response requires a **modified sender**. The
victim runs completely unmodified toxcore. This is why the bug is adversarial and
client-independent: it corrupts memory inside the victim's `tox_iterate`
regardless of how the client application dispatches events — the entire chain is
internal to toxcore, driven only by the incoming packet, with no application
callback on the stack.

## Why fuzzing did not find it

The end-to-end fuzzer drives a single `Tox` and never forms an NGC group between
two peers. A GC-handler fuzzer that blasts random fragment payloads cannot reach
the bug either: the terminating fragment only triggers `peer_add` if the
reassembled bytes form a *valid* `GP_SYNC_RESPONSE` announce that passes
`gca_unpack_announce` and `gca_is_valid_announce` (well-formed public key,
`ip_port`/TCP-relay fields) and names a *new* peer — a target random bytes
essentially never hit. The required state — a confirmed two-peer group plus a
deliberately fragmented, valid, new-peer sync response — is unreachable by those
harnesses but catchable by AddressSanitizer once reached.

## Reproduction

**Fast regression test:** `toxcore/repro_gc_fragment_uaf_test.cc` forms a real
two-peer NGC group, then from the attacker sends a fragmented `GP_SYNC_RESPONSE`
(a single valid announce for a fresh public key, padded past the 500-byte chunk
size to force fragmentation) via the real send path
(`gcc_send_lossless_packet_fragments`). Under `--config=asan` the unmodified code
aborts with `heap-use-after-free ... in gc_send_message_ack` reached from
`handle_gc_packet_fragment`; with the fix it passes.

**Live end-to-end:** `toxcore/gc_fragment_uaf_attacker.c` drives a real `Tox`
over the network against a separate, locally-run buffered-events client (the
`rs-toxcore-c` groupbot) built with `--config=asan`. The attacker befriends the
bot, creates an NGC group and invites the bot (the bot accepts invites from its
friend 0 via its own logic), then sends the crafted fragmented sync response.
ASan reports the use-after-free inside the victim's `tox_iterate_internal` ->
`networking_poll` -> `handle_gc_udp_packet` -> `handle_gc_lossless_packet` ->
`handle_gc_packet_fragment` -> `gc_send_message_ack`, freed via
`mem_vrealloc` <- `peer_add` <- `unpack_gc_sync_announce` <-
`handle_gc_sync_response` <- `handle_gc_lossless_helper` <-
`gcc_handle_packet_fragment` <- `handle_gc_packet_fragment` (the same frame that
then dereferences the stale pointer).

## Fix

Re-fetch `gconn` after `gcc_handle_packet_fragment` and null-check before
dereferencing it, keyed off the sender's public key (peer numbers can change
across peer-add), exactly as `handle_gc_lossless_packet` already does after its
own handler call (`toxcore/group_chats.c`, `handle_gc_packet_fragment`):

```c
    const int frag_ret = gcc_handle_packet_fragment(c, chat, peer_number, gconn, data, length,
                                                     packet_type, message_id, userdata);

    if (frag_ret == -1) {
        return false;
    }

    /* When the sequence completes, gcc_handle_packet_fragment() runs the
     * reassembled packet's handler, which can add a peer and realloc
     * chat->group, leaving gconn dangling. Re-fetch it before dereferencing it
     * again, exactly as handle_gc_lossless_packet() does after its own handler
     * call (peer numbers can change from peer add operations). */
    peer_number = get_peer_number_of_enc_pk(chat, sender_pk, false);
    gconn = get_gc_connection(chat, peer_number);

    if (gconn == nullptr) {
        return true;
    }

    if (frag_ret == 0) {
        gc_send_message_ack(chat, gconn, message_id, GR_ACK_RECV);
    }

    gconn->last_received_packet_time = mono_time_get(chat->mono_time);
```

`sender_pk` is threaded in from the sole caller, `handle_gc_lossless_packet`,
which already has it. With the fix, the fast test passes under ASan and the
ASan-built live client survives the identical sustained attack with no report.

## Test-only scaffolding

The live reproducer `other`-style attacker (`toxcore/gc_fragment_uaf_attacker.c`)
is a modified-toxcore binary that calls internal functions
(`gcc_send_lossless_packet_fragments`, `gca_pack_announce`) to craft the
fragmented sync response. It is a reproduction aid, not part of the production
fix, which is solely the `handle_gc_packet_fragment` re-fetch.
