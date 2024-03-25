/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022-2025 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_TOX_TIME_H
#define C_TOXCORE_TOXCORE_TOX_TIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tox_attributes.h"
#include "tox_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Tox_Time_Funcs Tox_Time_Funcs;

typedef struct Tox_Time Tox_Time;

Tox_Time *tox_time_new(const Tox_Time_Funcs *_Nonnull funcs, void *_Nullable user_data, const Tox_Memory *_Nonnull mem);

void tox_time_free(Tox_Time *_Nullable tm);

uint64_t tox_time_monotonic(const Tox_Time *_Nonnull tm);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_TOX_TIME_H */
