/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2025 The TokTok team.
 * Copyright © 2013 Tox project.
 */

#include "mem.h"

#include <stdlib.h>
#include <string.h>

#include "ccompat.h"
#include "tox_memory.h"
#include "tox_memory_impl.h"

static void *sys_malloc(void *_Nullable obj, uint32_t size)
{
    return malloc(size);
}

static void *sys_realloc(void *_Nullable obj, void *_Nullable ptr, uint32_t size)
{
    return realloc(ptr, size);
}

static void sys_free(void *_Nullable obj, void *_Nullable ptr)
{
    free(ptr);
}

static const Tox_Memory_Funcs system_memory_funcs = {
    sys_malloc,
    sys_realloc,
    sys_free,
};

static const Memory system_memory_obj = {&system_memory_funcs, NULL};

/*@
  @ assigns \result \from &system_memory_obj;
  @*/
const Memory *system_memory(void)
{
    return &system_memory_obj;
}

void *mem_balloc(const Memory *mem, uint32_t size)
{
    void *const ptr = tox_memory_malloc(mem, size);
    return ptr;
}

void *mem_brealloc(const Memory *mem, void *ptr, uint32_t size)
{
    void *const new_ptr = tox_memory_realloc(mem, ptr, size);
    return new_ptr;
}

void *mem_alloc(const Memory *mem, uint32_t size)
{
    void *const ptr = tox_memory_malloc(mem, size);
    if (ptr != nullptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *mem_valloc(const Memory *mem, uint32_t nmemb, uint32_t size)
{
    const uint32_t bytes = nmemb * size;

    if (size != 0 && bytes / size != nmemb) {
        return nullptr;
    }

    void *const ptr = tox_memory_malloc(mem, bytes);
    if (ptr != nullptr) {
        memset(ptr, 0, bytes);
    }
    return ptr;
}

void *mem_vrealloc(const Memory *mem, void *ptr, uint32_t nmemb, uint32_t size)
{
    const uint32_t bytes = nmemb * size;

    if (size != 0 && bytes / size != nmemb) {
        return nullptr;
    }

    void *const new_ptr = tox_memory_realloc(mem, ptr, bytes);
    return new_ptr;
}

void mem_delete(const Memory *mem, void *ptr)
{
    tox_memory_dealloc(mem, ptr);
}
