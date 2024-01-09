/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_EVENTS_EVENTS_ALLOC_H
#define C_TOXCORE_TOXCORE_EVENTS_EVENTS_ALLOC_H

#include "../attributes.h"
#include "../bin_pack.h"
#include "../bin_unpack.h"
#include "../tox_events.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Tox_Events {
    Tox_Event_Conference_Connected *conference_connected;
    uint32_t conference_connected_size;
    uint32_t conference_connected_capacity;

    Tox_Event_Conference_Invite *conference_invite;
    uint32_t conference_invite_size;
    uint32_t conference_invite_capacity;

    Tox_Event_Conference_Message *conference_message;
    uint32_t conference_message_size;
    uint32_t conference_message_capacity;

    Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed;
    uint32_t conference_peer_list_changed_size;
    uint32_t conference_peer_list_changed_capacity;

    Tox_Event_Conference_Peer_Name *conference_peer_name;
    uint32_t conference_peer_name_size;
    uint32_t conference_peer_name_capacity;

    Tox_Event_Conference_Title *conference_title;
    uint32_t conference_title_size;
    uint32_t conference_title_capacity;

    Tox_Event_File_Chunk_Request *file_chunk_request;
    uint32_t file_chunk_request_size;
    uint32_t file_chunk_request_capacity;

    Tox_Event_File_Recv *file_recv;
    uint32_t file_recv_size;
    uint32_t file_recv_capacity;

    Tox_Event_File_Recv_Chunk *file_recv_chunk;
    uint32_t file_recv_chunk_size;
    uint32_t file_recv_chunk_capacity;

    Tox_Event_File_Recv_Control *file_recv_control;
    uint32_t file_recv_control_size;
    uint32_t file_recv_control_capacity;

    Tox_Event_Friend_Connection_Status *friend_connection_status;
    uint32_t friend_connection_status_size;
    uint32_t friend_connection_status_capacity;

    Tox_Event_Friend_Lossless_Packet *friend_lossless_packet;
    uint32_t friend_lossless_packet_size;
    uint32_t friend_lossless_packet_capacity;

    Tox_Event_Friend_Lossy_Packet *friend_lossy_packet;
    uint32_t friend_lossy_packet_size;
    uint32_t friend_lossy_packet_capacity;

    Tox_Event_Friend_Message *friend_message;
    uint32_t friend_message_size;
    uint32_t friend_message_capacity;

    Tox_Event_Friend_Name *friend_name;
    uint32_t friend_name_size;
    uint32_t friend_name_capacity;

    Tox_Event_Friend_Read_Receipt *friend_read_receipt;
    uint32_t friend_read_receipt_size;
    uint32_t friend_read_receipt_capacity;

    Tox_Event_Friend_Request *friend_request;
    uint32_t friend_request_size;
    uint32_t friend_request_capacity;

    Tox_Event_Friend_Status *friend_status;
    uint32_t friend_status_size;
    uint32_t friend_status_capacity;

    Tox_Event_Friend_Status_Message *friend_status_message;
    uint32_t friend_status_message_size;
    uint32_t friend_status_message_capacity;

    Tox_Event_Friend_Typing *friend_typing;
    uint32_t friend_typing_size;
    uint32_t friend_typing_capacity;

    Tox_Event_Self_Connection_Status *self_connection_status;
    uint32_t self_connection_status_size;
    uint32_t self_connection_status_capacity;
};

typedef struct Tox_Events_State {
    Tox_Err_Events_Iterate error;
    Tox_Events *events;
} Tox_Events_State;

void tox_events_handle_self_connection_status(Tox *tox, Tox_Connection connection_status, void *user_data);
void tox_events_handle_friend_name(
        Tox *tox, Tox_Friend_Number friend_number,
        const uint8_t name[], size_t length, void *user_data);
void tox_events_handle_friend_status_message(
        Tox *tox, Tox_Friend_Number friend_number,
        const uint8_t message[], size_t length, void *user_data);
void tox_events_handle_friend_status(
        Tox *tox, Tox_Friend_Number friend_number, Tox_User_Status status, void *user_data);
void tox_events_handle_friend_connection_status(
        Tox *tox, Tox_Friend_Number friend_number, Tox_Connection connection_status, void *user_data);
void tox_events_handle_friend_typing(
        Tox *tox, Tox_Friend_Number friend_number, bool typing, void *user_data);
void tox_events_handle_friend_read_receipt(
        Tox *tox, Tox_Friend_Number friend_number, Tox_Friend_Message_Id message_id, void *user_data);
void tox_events_handle_friend_request(
        Tox *tox, const uint8_t public_key[TOX_PUBLIC_KEY_SIZE],
        const uint8_t message[], size_t length,
        void *user_data);
void tox_events_handle_friend_message(
        Tox *tox, Tox_Friend_Number friend_number, Tox_Message_Type type,
        const uint8_t message[], size_t length, void *user_data);
void tox_events_handle_file_recv_control(
        Tox *tox, Tox_Friend_Number friend_number, Tox_File_Number file_number, Tox_File_Control control,
        void *user_data);
void tox_events_handle_file_chunk_request(
        Tox *tox, Tox_Friend_Number friend_number, Tox_File_Number file_number, uint64_t position,
        size_t length, void *user_data);
void tox_events_handle_file_recv(
        Tox *tox, Tox_Friend_Number friend_number, Tox_File_Number file_number, uint32_t kind, uint64_t file_size,
        const uint8_t filename[], size_t filename_length, void *user_data);
void tox_events_handle_file_recv_chunk(
        Tox *tox, Tox_Friend_Number friend_number, Tox_File_Number file_number, uint64_t position,
        const uint8_t data[], size_t length, void *user_data);
void tox_events_handle_conference_invite(
        Tox *tox, Tox_Friend_Number friend_number, Tox_Conference_Type type,
        const uint8_t cookie[], size_t length, void *user_data);
void tox_events_handle_conference_connected(Tox *tox, Tox_Conference_Number conference_number, void *user_data);
void tox_events_handle_conference_message(
        Tox *tox, Tox_Conference_Number conference_number, Tox_Conference_Peer_Number peer_number,
        Tox_Message_Type type, const uint8_t message[], size_t length, void *user_data);
void tox_events_handle_conference_title(
        Tox *tox, Tox_Conference_Number conference_number, Tox_Conference_Peer_Number peer_number,
        const uint8_t title[], size_t length, void *user_data);
void tox_events_handle_conference_peer_name(
        Tox *tox, Tox_Conference_Number conference_number, Tox_Conference_Peer_Number peer_number,
        const uint8_t name[], size_t length, void *user_data);
void tox_events_handle_conference_peer_list_changed(Tox *tox, Tox_Conference_Number conference_number, void *user_data);
void tox_events_handle_friend_lossy_packet(
        Tox *tox, Tox_Friend_Number friend_number,
        const uint8_t data[], size_t length,
        void *user_data);
void tox_events_handle_friend_lossless_packet(
        Tox *tox, Tox_Friend_Number friend_number,
        const uint8_t data[], size_t length,
        void *user_data);


void tox_events_clear_conference_connected(Tox_Events *events);
void tox_events_clear_conference_invite(Tox_Events *events);
void tox_events_clear_conference_message(Tox_Events *events);
void tox_events_clear_conference_peer_list_changed(Tox_Events *events);
void tox_events_clear_conference_peer_name(Tox_Events *events);
void tox_events_clear_conference_title(Tox_Events *events);
void tox_events_clear_file_chunk_request(Tox_Events *events);
void tox_events_clear_file_recv_chunk(Tox_Events *events);
void tox_events_clear_file_recv_control(Tox_Events *events);
void tox_events_clear_file_recv(Tox_Events *events);
void tox_events_clear_friend_connection_status(Tox_Events *events);
void tox_events_clear_friend_lossless_packet(Tox_Events *events);
void tox_events_clear_friend_lossy_packet(Tox_Events *events);
void tox_events_clear_friend_message(Tox_Events *events);
void tox_events_clear_friend_name(Tox_Events *events);
void tox_events_clear_friend_read_receipt(Tox_Events *events);
void tox_events_clear_friend_request(Tox_Events *events);
void tox_events_clear_friend_status_message(Tox_Events *events);
void tox_events_clear_friend_status(Tox_Events *events);
void tox_events_clear_friend_typing(Tox_Events *events);
void tox_events_clear_self_connection_status(Tox_Events *events);


bool tox_events_pack_conference_connected(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_conference_invite(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_conference_message(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_conference_peer_list_changed(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_conference_peer_name(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_conference_title(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_file_chunk_request(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_file_recv_chunk(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_file_recv_control(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_file_recv(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_connection_status(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_lossless_packet(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_lossy_packet(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_message(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_name(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_read_receipt(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_request(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_status_message(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_status(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_friend_typing(const Tox_Events *events, Bin_Pack *bp);
bool tox_events_pack_self_connection_status(const Tox_Events *events, Bin_Pack *bp);

bool tox_events_pack(const Tox_Events *events, Bin_Pack *bp);

bool tox_events_unpack_conference_connected(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_conference_invite(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_conference_message(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_conference_peer_list_changed(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_conference_peer_name(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_conference_title(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_file_chunk_request(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_file_recv_chunk(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_file_recv_control(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_file_recv(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_connection_status(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_lossless_packet(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_lossy_packet(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_message(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_name(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_read_receipt(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_request(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_status_message(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_status(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_friend_typing(Tox_Events *events, Bin_Unpack *bu);
bool tox_events_unpack_self_connection_status(Tox_Events *events, Bin_Unpack *bu);

bool tox_events_unpack(Tox_Events *events, Bin_Unpack *bu);

non_null()
Tox_Events_State *tox_events_alloc(void *user_data);

#ifdef __cplusplus
}
#endif

#endif // C_TOXCORE_TOXCORE_EVENTS_EVENTS_ALLOC_H
