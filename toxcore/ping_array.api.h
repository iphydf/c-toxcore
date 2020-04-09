/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2020 The TokTok team.
 * Copyright © 2013 Tox project.
 */

#include <stddef.h>
#include <stdint.h>

class Mono_Time;

/**
 * Implementation of an efficient array to store that we pinged something.
 */
class Ping_Array {

/**
 * Initialize a #this.
 *
 * @param size represents the total size of the array and should be a power of 2.
 * @param timeout represents the maximum timeout in seconds for the entry.
 *
 * @return 0 on success, -1 on failure.
 */
static this new(uint32_t size, uint32_t timeout);

/**
 * Free all the allocated memory in a #this.
 */
void kill(void);

/**
 * Add a data with length to the #this list and return a `ping_id`.
 *
 * @return ping_id on success, 0 on failure.
 */
uint64_t add(const Mono_Time *mono_time, const uint8_t data[length]);

/**
 * Check if #ping_id is valid and not timed out.
 *
 * On success, copies the data into data of length,
 *
 * @return length of data copied on success, -1 on failure.
 */
int32_t check(const Mono_Time *mono_time, uint8_t data[length], uint64_t ping_id);

}
