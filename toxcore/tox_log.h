/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022-2025 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_TOX_LOG_H
#define C_TOXCORE_TOXCORE_TOX_LOG_H

#include <stdint.h>     // uint32_t

#include "tox_attributes.h"
#include "tox_log_level.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TOX_MEMORY_DEFINED
#define TOX_MEMORY_DEFINED
typedef struct Tox_Memory Tox_Memory;
#endif /* TOX_MEMORY_DEFINED */

typedef struct Tox_Log_Funcs Tox_Log_Funcs;

typedef struct Tox_Log Tox_Log;

Tox_Log *tox_log_new(const Tox_Log_Funcs *_Nonnull funcs, void *_Nullable user_data, const Tox_Memory *_Nonnull mem);

void tox_log_free(Tox_Log *_Nullable log);

void tox_log_log(
    const Tox_Log *_Nonnull log, Tox_Log_Level level,
    const char *_Nonnull file, uint32_t line, const char *_Nonnull func,
    const char *_Nonnull message);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_TOX_LOG_H */
