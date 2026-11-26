/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2026 The TokTok team.
 */
#include <gtest/gtest.h>

#include <cstdint>

#include "tox.h"

namespace {

// Creating every conference there can be takes a while: each new one looks
// through all the others for a free slot.
TEST(Conference, NewStopsAtTheMostTheirCountHolds)
{
    Tox *tox = tox_new(nullptr, nullptr);
    ASSERT_NE(tox, nullptr);

    for (std::uint32_t i = 0; i < UINT16_MAX; ++i) {
        ASSERT_EQ(tox_conference_new(tox, nullptr), i);
    }

    Tox_Err_Conference_New err;
    EXPECT_EQ(tox_conference_new(tox, &err), UINT32_MAX);
    EXPECT_EQ(err, TOX_ERR_CONFERENCE_NEW_INIT);
    EXPECT_EQ(tox_conference_get_chatlist_size(tox), UINT16_MAX);

    tox_kill(tox);
}

}  // namespace
