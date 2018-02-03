/*
 * Copyright © 2016-2018 The TokTok team.
 * Copyright © 2013-2015 Tox project.
 *
 * This file is part of Tox, the free peer to peer instant messenger.
 *
 * Tox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Tox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tox.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "rtp.h"

#include "bwcontroller.h"

#include "../toxcore/Messenger.h"
#include "../toxcore/logger.h"
#include "../toxcore/util.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>


size_t rtp_header_pack(uint8_t *const rdata, const struct RTPHeader *header)
{
    uint8_t *p = rdata;
    *p++ = (header->protocol_version & 3) << 6
           | (header->pe & 1) << 5
           | (header->xe & 1) << 4
           | (header->cc & 0xf);
    *p++ = (header->ma & 1) << 7
           | (header->pt & 0x7f);

    p += net_pack_u16(p, header->sequnum);
    p += net_pack_u32(p, header->timestamp);
    p += net_pack_u32(p, header->ssrc);
    p += net_pack_u64(p, header->flags);
    p += net_pack_u32(p, header->offset_full);
    p += net_pack_u32(p, header->data_length_full);
    p += net_pack_u32(p, header->received_length_full);

    for (size_t i = 0; i < RTP_PADDING_FIELDS; i++) {
        p += net_pack_u32(p, 0);
    }

    p += net_pack_u16(p, header->offset_lower);
    p += net_pack_u16(p, header->data_length_lower);
    assert(p == rdata + RTP_HEADER_SIZE);
    return p - rdata;
}


size_t rtp_header_unpack(const uint8_t *data, struct RTPHeader *header)
{
    const uint8_t *p = data;
    header->protocol_version = (*p >> 6) & 3;
    header->pe = (*p >> 5) & 1;
    header->xe = (*p >> 4) & 1;
    header->cc = *p & 0xf;
    ++p;

    header->ma = (*p >> 7) & 1;
    header->pt = *p & 0x7f;
    ++p;

    p += net_unpack_u16(p, &header->sequnum);
    p += net_unpack_u32(p, &header->timestamp);
    p += net_unpack_u32(p, &header->ssrc);
    p += net_unpack_u64(p, &header->flags);
    p += net_unpack_u32(p, &header->offset_full);
    p += net_unpack_u32(p, &header->data_length_full);
    p += net_unpack_u32(p, &header->received_length_full);

    p += sizeof(uint32_t) * RTP_PADDING_FIELDS;

    p += net_unpack_u16(p, &header->offset_lower);
    p += net_unpack_u16(p, &header->data_length_lower);
    assert(p == data + RTP_HEADER_SIZE);
    return p - data;
}


int handle_rtp_packet(Messenger *m, uint32_t friendnumber, const uint8_t *data, uint16_t length, void *object);


RTPSession *rtp_new(int payload_type, Messenger *m, uint32_t friendnumber,
                    BWController *bwc, void *cs,
                    int (*mcb)(void *, struct RTPMessage *))
{
    assert(mcb);
    assert(cs);
    assert(m);

    RTPSession *retu = (RTPSession *)calloc(1, sizeof(RTPSession));

    if (!retu) {
        LOGGER_WARNING(m->log, "Alloc failed! Program might misbehave!");
        return nullptr;
    }

    retu->ssrc = random_u32();
    retu->payload_type = payload_type;

    retu->m = m;
    retu->friend_number = friendnumber;

    /* Also set payload type as prefix */

    retu->bwc = bwc;
    retu->cs = cs;
    retu->mcb = mcb;

    if (-1 == rtp_allow_receiving(retu)) {
        LOGGER_WARNING(m->log, "Failed to start rtp receiving mode");
        free(retu);
        return nullptr;
    }

    return retu;
}
void rtp_kill(RTPSession *session)
{
    if (!session) {
        return;
    }

    LOGGER_DEBUG(session->m->log, "Terminated RTP session: %p", session);

    rtp_stop_receiving(session);
    free(session);
}
int rtp_allow_receiving(RTPSession *session)
{
    if (session == nullptr) {
        return -1;
    }

    if (m_callback_rtp_packet(session->m, session->friend_number, session->payload_type,
                              handle_rtp_packet, session) == -1) {
        LOGGER_WARNING(session->m->log, "Failed to register rtp receive handler");
        return -1;
    }

    LOGGER_DEBUG(session->m->log, "Started receiving on session: %p", session);
    return 0;
}
int rtp_stop_receiving(RTPSession *session)
{
    if (session == nullptr) {
        return -1;
    }

    m_callback_rtp_packet(session->m, session->friend_number, session->payload_type, nullptr, nullptr);

    LOGGER_DEBUG(session->m->log, "Stopped receiving on session: %p", session);
    return 0;
}
int rtp_send_data(RTPSession *session, const uint8_t *data, uint16_t length, Logger *log)
{
    if (!session) {
        LOGGER_ERROR(log, "No session!");
        return -1;
    }

    VLA(uint8_t, rdata, length + RTP_HEADER_SIZE + 1);
    memset(rdata, 0, SIZEOF_VLA(rdata));

    rdata[0] = session->payload_type;

    struct RTPHeader header = {0};

    header.protocol_version = 2;
    header.pe = 0;
    header.xe = 0;
    header.cc = 0;

    header.ma = 0;
    header.pt = session->payload_type % 128;

    header.sequnum = session->sequnum;
    header.timestamp = current_time_monotonic();
    header.ssrc = session->ssrc;

    header.offset_lower = 0;
    header.data_length_lower = length;

    if (MAX_CRYPTO_DATA_SIZE > length + RTP_HEADER_SIZE + 1) {

        /**
         * The length is lesser than the maximum allowed length (including header)
         * Send the packet in single piece.
         */

        rtp_header_pack(rdata + 1, &header);
        memcpy(rdata + 1 + RTP_HEADER_SIZE, data, length);

        if (-1 == m_send_custom_lossy_packet(session->m, session->friend_number, rdata, SIZEOF_VLA(rdata))) {
            LOGGER_WARNING(session->m->log, "RTP send failed (len: %d)! std error: %s", SIZEOF_VLA(rdata), strerror(errno));
        }
    } else {

        /**
         * The length is greater than the maximum allowed length (including header)
         * Send the packet in multiple pieces.
         */

        uint16_t sent = 0;
        uint16_t piece = MAX_CRYPTO_DATA_SIZE - (RTP_HEADER_SIZE + 1);

        while ((length - sent) + RTP_HEADER_SIZE + 1 > MAX_CRYPTO_DATA_SIZE) {
            rtp_header_pack(rdata + 1, &header);
            memcpy(rdata + 1 + RTP_HEADER_SIZE, data + sent, piece);

            if (-1 == m_send_custom_lossy_packet(session->m, session->friend_number,
                                                 rdata, piece + RTP_HEADER_SIZE + 1)) {
                LOGGER_WARNING(session->m->log, "RTP send failed (len: %d)! std error: %s",
                               piece + RTP_HEADER_SIZE + 1, strerror(errno));
            }

            sent += piece;
            header.offset_lower = sent;
        }

        /* Send remaining */
        piece = length - sent;

        if (piece) {
            rtp_header_pack(rdata + 1, &header);
            memcpy(rdata + 1 + RTP_HEADER_SIZE, data + sent, piece);

            if (-1 == m_send_custom_lossy_packet(session->m, session->friend_number, rdata,
                                                 piece + RTP_HEADER_SIZE + 1)) {
                LOGGER_WARNING(session->m->log, "RTP send failed (len: %d)! std error: %s",
                               piece + RTP_HEADER_SIZE + 1, strerror(errno));
            }
        }
    }

    session->sequnum ++;
    return 0;
}


static bool chloss(const RTPSession *session, const struct RTPHeader *header)
{
    if (header->timestamp < session->rtimestamp) {
        uint16_t hosq, lost = 0;

        hosq = header->sequnum;

        lost = (hosq > session->rsequnum) ?
               (session->rsequnum + 65535) - hosq :
               session->rsequnum - hosq;

        fprintf(stderr, "Lost packet\n");

        while (lost --) {
            bwc_add_lost(session->bwc, 0);
        }

        return true;
    }

    return false;
}
static struct RTPMessage *new_message(size_t allocate_len, const uint8_t *data, uint16_t data_length)
{
    assert(allocate_len >= data_length);

    struct RTPMessage *msg = (struct RTPMessage *)calloc(sizeof(struct RTPMessage) +
                             (allocate_len - RTP_HEADER_SIZE), 1);

    if (msg == nullptr) {
        return nullptr;
    }

    msg->len = data_length - RTP_HEADER_SIZE;
    rtp_header_unpack(data, &msg->header);
    memcpy(msg->data, data + RTP_HEADER_SIZE, allocate_len - RTP_HEADER_SIZE);

    return msg;
}

int handle_rtp_packet(Messenger *m, uint32_t friendnumber, const uint8_t *data, uint16_t length, void *object)
{
    (void) m;
    (void) friendnumber;

    RTPSession *session = (RTPSession *)object;

    data ++;
    length--;

    if (!session || length < RTP_HEADER_SIZE) {
        LOGGER_WARNING(m->log, "No session or invalid length of received buffer!");
        return -1;
    }

    struct RTPHeader header;

    rtp_header_unpack(data, &header);

    if (header.pt != session->payload_type % 128) {
        LOGGER_WARNING(m->log, "Invalid payload type with the session");
        return -1;
    }

    if (header.offset_lower >= header.data_length_lower) {
        /* Never allow this case to happen */
        return -1;
    }

    bwc_feed_avg(session->bwc, length);

    if (header.data_length_lower == length - RTP_HEADER_SIZE) {
        /* The message is sent in single part */

        /* Only allow messages which have arrived in order;
         * drop late messages
         */
        if (chloss(session, &header)) {
            return 0;
        }

        /* Message is not late; pick up the latest parameters */
        session->rsequnum = header.sequnum;
        session->rtimestamp = header.timestamp;

        bwc_add_recv(session->bwc, length);

        /* Invoke processing of active multiparted message */
        if (session->mp) {
            if (session->mcb) {
                session->mcb(session->cs, session->mp);
            } else {
                free(session->mp);
            }

            session->mp = nullptr;
        }

        /* The message came in the allowed time;
         * process it only if handler for the session is present.
         */

        if (!session->mcb) {
            return 0;
        }

        return session->mcb(session->cs, new_message(length, data, length));
    }

    /* The message is sent in multiple parts */

    if (session->mp) {
        /* There are 2 possible situations in this case:
         *      1) being that we got the part of already processing message.
         *      2) being that we got the part of a new/old message.
         *
         * We handle them differently as we only allow a single multiparted
         * processing message
         */

        if (session->mp->header.sequnum == header.sequnum &&
                session->mp->header.timestamp == header.timestamp) {
            /* First case */

            /* Make sure we have enough allocated memory */
            if (session->mp->header.data_length_lower - session->mp->len < length - RTP_HEADER_SIZE ||
                    session->mp->header.data_length_lower <= header.offset_lower) {
                /* There happened to be some corruption on the stream;
                 * continue wihtout this part
                 */
                return 0;
            }

            memcpy(session->mp->data + header.offset_lower, data + RTP_HEADER_SIZE,
                   length - RTP_HEADER_SIZE);

            session->mp->len += length - RTP_HEADER_SIZE;

            bwc_add_recv(session->bwc, length);

            if (session->mp->len == session->mp->header.data_length_lower) {
                /* Received a full message; now push it for the further
                 * processing.
                 */
                if (session->mcb) {
                    session->mcb(session->cs, session->mp);
                } else {
                    free(session->mp);
                }

                session->mp = nullptr;
            }
        } else {
            /* Second case */

            if (session->mp->header.timestamp > header.timestamp) {
                /* The received message part is from the old message;
                 * discard it.
                 */
                return 0;
            }

            /* Measure missing parts of the old message */
            bwc_add_lost(session->bwc,
                         (session->mp->header.data_length_lower - session->mp->len) +

                         /* Must account sizes of rtp headers too */
                         ((session->mp->header.data_length_lower - session->mp->len) /
                          MAX_CRYPTO_DATA_SIZE) * RTP_HEADER_SIZE);

            /* Push the previous message for processing */
            if (session->mcb) {
                session->mcb(session->cs, session->mp);
            } else {
                free(session->mp);
            }

            session->mp = nullptr;
            goto NEW_MULTIPARTED;
        }
    } else {
        /* In this case threat the message as if it was received in order
         */

        /* This is also a point for new multiparted messages */
NEW_MULTIPARTED:

        /* Only allow messages which have arrived in order;
         * drop late messages
         */
        if (chloss(session, &header)) {
            return 0;
        }

        /* Message is not late; pick up the latest parameters */
        session->rsequnum = header.sequnum;
        session->rtimestamp = header.timestamp;

        bwc_add_recv(session->bwc, length);

        /* Again, only store message if handler is present
         */
        if (session->mcb) {
            session->mp = new_message(header.data_length_lower + RTP_HEADER_SIZE, data, length);
            memmove(session->mp->data + header.offset_lower, session->mp->data, session->mp->len);
        }
    }

    return 0;
}
