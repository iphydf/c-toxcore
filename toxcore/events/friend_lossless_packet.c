/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022 The TokTok team.
 */

#include "events_alloc.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../bin_pack.h"
#include "../bin_unpack.h"
#include "../ccompat.h"
#include "../tox.h"
#include "../tox_events.h"
#include "../tox_private.h"
#include "event_macros.h"


/*****************************************************
 *
 * :: struct and accessors
 *
 *****************************************************/


struct Tox_Event_Friend_Lossless_Packet {
    uint32_t friend_number;
    uint8_t *data;
    uint32_t data_length;
};

EV_ACCESS_VALUE(Friend_Lossless_Packet, friend_lossless_packet, uint32_t, friend_number)
EV_ACCESS_ARRAY(Friend_Lossless_Packet, friend_lossless_packet, uint8_t, data)

non_null()
static void tox_event_friend_lossless_packet_construct(Tox_Event_Friend_Lossless_Packet *friend_lossless_packet)
{
    *friend_lossless_packet = (Tox_Event_Friend_Lossless_Packet) {
        0
    };
}
non_null()
static void tox_event_friend_lossless_packet_destruct(Tox_Event_Friend_Lossless_Packet *friend_lossless_packet, const Memory *mem)
{
    mem_delete(mem, friend_lossless_packet->data);
}

bool tox_event_friend_lossless_packet_pack(
    const Tox_Event_Friend_Lossless_Packet *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_FRIEND_LOSSLESS_PACKET)
           && bin_pack_array(bp, 2)
           && bin_pack_u32(bp, event->friend_number)
           && bin_pack_bin(bp, event->data, event->data_length);
}

non_null()
static bool tox_event_friend_lossless_packet_unpack_into(
    Tox_Event_Friend_Lossless_Packet *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    if (!bin_unpack_array_fixed(bu, 2, nullptr)) {
        return false;
    }

    return bin_unpack_u32(bu, &event->friend_number)
           && bin_unpack_bin(bu, &event->data, &event->data_length);
}

EV_FUNCS(Friend_Lossless_Packet, friend_lossless_packet, FRIEND_LOSSLESS_PACKET)


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_friend_lossless_packet(Tox *tox, uint32_t friend_number, const uint8_t *data, size_t length,
        void *user_data)
{
    Tox_Event_Friend_Lossless_Packet *friend_lossless_packet = tox_event_friend_lossless_packet_alloc(user_data);

    if (friend_lossless_packet == nullptr) {
        return;
    }

    const Tox_System *sys = tox_get_system(tox);

    tox_event_friend_lossless_packet_set_friend_number(friend_lossless_packet, friend_number);
    tox_event_friend_lossless_packet_set_data(friend_lossless_packet, data, length, sys->mem);
}
