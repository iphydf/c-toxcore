/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022-2026 The TokTok team.
 */

/**
 * WARNING: This is an experimental API and is subject to change.
 *
 * At this point, it probably won't change very much anymore, but we may have
 * small breaking changes before a stable release.
 */

#ifndef C_TOXCORE_TOXCORE_TOX_EVENTS_H
#define C_TOXCORE_TOXCORE_TOX_EVENTS_H

#include <stdbool.h>
#include <stdint.h>

#include "tox.h"
#include "tox_attributes.h"
#include "tox_event.h"
#include "tox_options.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Tox_Iterate_Options Tox_Iterate_Options;

/**
 * Container object for all Tox core events.
 *
 * This is an immutable object once created.
 */
typedef struct Tox_Events Tox_Events;

uint32_t tox_events_get_size(const Tox_Events *_Nullable events);
const Tox_Event *_Nullable tox_events_get(const Tox_Events *_Nullable events, uint32_t index);

/**
 * Clear all legacy callbacks on the Tox instance.
 *
 * This ensures that when using the event system, no legacy callbacks
 * are invoked during dispatch, unless they are set after this call.
 */
void tox_events_init(Tox *_Nonnull tox);

typedef enum Tox_Err_Events_Iterate {
    /**
     * The function returned successfully.
     */
    TOX_ERR_EVENTS_ITERATE_OK,

    /**
     * The function failed to allocate enough memory to store the events.
     *
     * Some events may still be stored if the return value is NULL. The events
     * object will always be valid (or NULL) but if this error code is set,
     * the function may have missed some events.
     */
    TOX_ERR_EVENTS_ITERATE_MALLOC,
} Tox_Err_Events_Iterate;

const char *_Nonnull tox_err_events_iterate_to_string(Tox_Err_Events_Iterate value);

/**
 * Run a single `tox_iterate` iteration and record all the events.
 *
 * If allocation of the top level events object fails, this returns NULL.
 * Otherwise it returns an object with the recorded events in it. If an
 * allocation fails while recording events, some events may be dropped.
 *
 * If `fail_hard` in @p options is `true`, any failure will result in NULL, so
 * all recorded events will be dropped.
 *
 * The result must be freed using `tox_events_free`.
 *
 * @param tox The Tox instance to iterate on.
 * @param options Options for the iteration. If NULL, default options are used.
 * @param error An error code. Will be set to OK on success.
 *
 * @return the recorded events structure.
 */
Tox_Events *_Nullable tox_events_iterate(
    Tox *_Nonnull tox,
    const Tox_Iterate_Options *_Nullable options,
    Tox_Err_Events_Iterate *_Nullable error);

/**
 * Dispatch all events in the events object to the registered callbacks in the
 * Tox instance.
 *
 * @param tox The Tox instance to dispatch to.
 * @param events The events object to dispatch.
 * @param user_data The user data to pass to the callbacks.
 */
void tox_events_dispatch(Tox *_Nonnull tox, Tox_Events *_Nullable events, void *_Nullable user_data);

/**
 * Frees all memory associated with the events structure.
 *
 * All pointers into this object and its sub-objects, including byte buffers,
 * will be invalid once this function returns.
 */
void tox_events_free(Tox_Events *_Nullable events);

uint32_t tox_events_bytes_size(const Tox_Events *_Nullable events);

typedef enum Tox_Err_Events_Get_Bytes {
    /**
     * The function returned successfully.
     */
    TOX_ERR_EVENTS_GET_BYTES_OK,

    /**
     * The buffer is smaller than the events' bytes: tox_events_bytes_size says
     * how big it needs to be.
     */
    TOX_ERR_EVENTS_GET_BYTES_TOO_SMALL,
} Tox_Err_Events_Get_Bytes;

const char *_Nonnull tox_err_events_get_bytes_to_string(Tox_Err_Events_Get_Bytes value);

/**
 * Writes the events into `bytes`, which is `bytes_size` bytes long.
 */
bool tox_events_get_bytes(const Tox_Events *_Nullable events, uint8_t bytes[_Nonnull], uint32_t bytes_size,
                          Tox_Err_Events_Get_Bytes *_Nullable error);

typedef struct Tox_System Tox_System;

Tox_Events *_Nullable tox_events_load(const Tox_System *_Nonnull sys, const uint8_t *_Nonnull bytes, uint32_t bytes_size);

bool tox_events_equal(const Tox_System *_Nonnull sys, const Tox_Events *_Nullable a, const Tox_Events *_Nullable b);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_TOX_EVENTS_H */
