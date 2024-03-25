/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022-2025 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_TOX_SYSTEM_H
#define C_TOXCORE_TOXCORE_TOX_SYSTEM_H

#include "tox_attributes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Tox_Log;
struct Tox_Memory;
struct Tox_Network;
struct Tox_Random;
struct Tox_Time;

/**
 * @brief Operating system functions used by Tox.
 *
 * This struct is opaque and generally shouldn't be used in clients, but in
 * combination with tox_system_impl.h, it allows tests to inject non-IO
 * (hermetic) versions of low level network, RNG, and time keeping functions.
 */
typedef struct Tox_System Tox_System;

Tox_System *tox_system_new(const struct Tox_Log *_Nonnull log, const struct Tox_Memory *_Nonnull mem, const struct Tox_Network *_Nonnull ns, const struct Tox_Random *_Nonnull rng, const struct Tox_Time *_Nonnull tm);

void tox_system_free(Tox_System *_Nullable sys);

const struct Tox_Log *tox_system_get_log(const Tox_System *_Nonnull sys);

const struct Tox_Memory *tox_system_get_memory(const Tox_System *_Nonnull sys);

const struct Tox_Network *tox_system_get_network(const Tox_System *_Nonnull sys);

const struct Tox_Random *tox_system_get_random(const Tox_System *_Nonnull sys);

const struct Tox_Time *tox_system_get_time(const Tox_System *_Nonnull sys);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_TOX_SYSTEM_H */
