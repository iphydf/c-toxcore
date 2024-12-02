/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2023-2024 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_SORT_TEST_UTIL_H
#define C_TOXCORE_TOXCORE_SORT_TEST_UTIL_H

#include "sort.h"

template <typename T>
constexpr Sort_Funcs sort_funcs()
{
    return {
        [](const void *object, const void *va, const void *vb) {
            const T *a = static_cast<const T *>(va);
            const T *b = static_cast<const T *>(vb);

            return *a < *b;
        },
        [](const void *arr, uint32_t index) -> const void * {
            const T *vec = static_cast<const T *>(arr);
            return &vec[index];
        },
        [](void *arr, uint32_t index, const void *val) {
            T *vec = static_cast<T *>(arr);
            const T *value = static_cast<const T *>(val);
            vec[index] = *value;
        },
        [](void *arr, uint32_t index, uint32_t size) -> void * {
            T *vec = static_cast<T *>(arr);
            return &vec[index];
        },
        [](const void *object, uint32_t size) -> void * { return new T[size]; },
        [](const void *object, void *arr, uint32_t size) {
            T *vec = static_cast<T *>(arr);
            delete[] vec;
        },
        [](const void *object, uint32_t size) -> int * { return new int[size]; },
        [](const void *object, int *arr, uint32_t size) { delete[] arr; },
    };
}

#endif  // C_TOXCORE_TOXCORE_SORT_TEST_UTIL_H
