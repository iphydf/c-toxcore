/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2023-2024 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_SORT_H
#define C_TOXCORE_TOXCORE_SORT_H

#include <stdbool.h>
#include <stdint.h>

#include "attributes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Compare elements with a less-than ordering: `a < b`. */
typedef bool sort_less_cb(const void *object, const void *a, const void *b);
/** @brief Get element from array at index. */
typedef const void *sort_get_cb(const void *arr, uint32_t index);
/** @brief Set element in array at index to new value (perform copy). */
typedef void sort_set_cb(void *arr, uint32_t index, const void *val);
/** @brief Get a sub-array at an index of a given size (mutable pointer).
 *
 * Used to index in the temporary array allocated by `sort_alloc_cb` and get
 * a sub-array for working memory.
 */
typedef void *sort_subarr_cb(void *arr, uint32_t index, uint32_t size);
/** @brief Allocate a new array of the element type.
 *
 * @param size The array size in elements of type T (not byte size). This value
 *   is always exactly the input array size as passed to `merge_sort`.
 */
typedef void *sort_alloc_vec_cb(const void *object, uint32_t size);
/** @brief Free the element type array. */
typedef void sort_delete_vec_cb(const void *object, void *arr, uint32_t size);
/** @brief Allocate a temporary array of `int` for sorting.
 *
 * @param size The array size in elements of type `int`.
 */
typedef int *sort_alloc_cb(const void *object, uint32_t size);
/** @brief Free the temporary array of `int`. */
typedef void sort_delete_cb(const void *object, int *arr, uint32_t size);

/** @brief Virtual function table for getting/setting elements in an array and
 * comparing them.
 *
 * Only the `less`, `alloc`, and `delete` functions get a `this`-pointer. We
 * assume that indexing in an array doesn't need any other information than the
 * array itself.
 *
 * For now, the `this`-pointer is const, because we assume sorting doesn't need
 * to mutate any state, but if necessary that can be changed in the future.
 */
typedef struct Sort_Funcs {
    sort_less_cb *less_callback;
    sort_get_cb *get_callback;
    sort_set_cb *set_callback;
    sort_subarr_cb *subarr_callback;
    sort_alloc_vec_cb *alloc_vec_callback;
    sort_delete_vec_cb *delete_vec_callback;
    sort_alloc_cb *alloc_callback;
    sort_delete_cb *delete_callback;
} Sort_Funcs;

/** @brief Non-recursive merge sort function to sort `arr[0...arr_size-1]`.
 *
 * Avoids `memcpy` and avoids treating elements as byte arrays. Instead, uses
 * callbacks to index in arrays and copy elements. This makes it quite a bit
 * slower than `qsort`, but works with elements that require special care when
 * being copied (e.g. if they are part of a graph or other data structure that
 * with pointers or other invariants).
 *
 * Allocates a single temporary array with the provided alloc callback, and
 * frees it at the end. This is significantly faster than an in-place
 * implementation.
 *
 * This could be made more efficient by providing range-copy functions instead
 * of calling the get/set callback for every element, but that increases code
 * complexity on the caller.
 *
 * Complexity:
 * - Space: `O(n) where n = array_size`.
 * - Time: `O(n * log n) where n = array_size`.
 *
 * @param[in,out] arr An array of type T.
 * @param arr_size Number of elements in @p arr (count, not byte size).
 * @param[in] object Comparator object.
 * @param[in] funcs Callback struct for elements of type T.
 */
non_null()
bool merge_sort(void *arr, uint32_t arr_size, const void *object, const Sort_Funcs *funcs);

non_null()
bool quick_sort(void *arr, uint32_t arr_size, const void *object, const Sort_Funcs *funcs);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_SORT_H */
