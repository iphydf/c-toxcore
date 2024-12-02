/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2023-2024 The TokTok team.
 */

#include "sort.h"

#include "attributes.h"
#include "ccompat.h"
#include "util.h"

non_null()
static void merge_sort_merge_back(
    void *arr,
    const void *l_arr, uint32_t l_arr_size,
    const void *r_arr, uint32_t r_arr_size,
    uint32_t left_start,
    const void *object, const Sort_Funcs *funcs)
{
    uint32_t li = 0;
    uint32_t ri = 0;
    uint32_t k = left_start;

    while (li < l_arr_size && ri < r_arr_size) {
        const void *l = funcs->get_callback(l_arr, li);
        const void *r = funcs->get_callback(r_arr, ri);
        // !(r < l) <=> (r >= l) <=> (l <= r)
        if (!funcs->less_callback(object, r, l)) {
            funcs->set_callback(arr, k, l);
            ++li;
        } else {
            funcs->set_callback(arr, k, r);
            ++ri;
        }
        ++k;
    }

    /* Copy the remaining elements, if there are any. */
    if (funcs->copy_callback != nullptr) {
        funcs->copy_callback(funcs->subarr_callback(arr, k), funcs->csubarr_callback(l_arr, li), l_arr_size - li);
        funcs->copy_callback(funcs->subarr_callback(arr, k), funcs->csubarr_callback(r_arr, ri), r_arr_size - ri);
    } else {
        while (li < l_arr_size) {
            funcs->set_callback(arr, k, funcs->get_callback(l_arr, li));
            ++li;
            ++k;
        }

        while (ri < r_arr_size) {
            funcs->set_callback(arr, k, funcs->get_callback(r_arr, ri));
            ++ri;
            ++k;
        }
    }
}

/** Function to merge the two haves `arr[left_start..mid]` and `arr[mid+1..right_end]` of array `arr[]`. */
non_null()
static void merge_sort_merge(
    void *arr, uint32_t left_start, uint32_t mid, uint32_t right_end, void *tmp,
    const void *object, const Sort_Funcs *funcs)
{
    const uint32_t l_arr_size = mid - left_start + 1;
    const uint32_t r_arr_size = right_end - mid;

    /* Temporary arrays, using the tmp buffer created in `merge_sort` below. */
    void *l_arr = funcs->subarr_callback(tmp, 0);
    void *r_arr = funcs->subarr_callback(tmp, l_arr_size);

    /* Copy data to temp arrays `l_arr[]` and `r_arr[]`. */
    if (funcs->copy_callback != nullptr) {
        funcs->copy_callback(l_arr, funcs->csubarr_callback(arr, left_start), l_arr_size);
        funcs->copy_callback(r_arr, funcs->csubarr_callback(arr, mid + 1), r_arr_size);
    } else {
        for (uint32_t i = 0; i < l_arr_size; ++i) {
            funcs->set_callback(l_arr, i, funcs->get_callback(arr, left_start + i));
        }
        for (uint32_t i = 0; i < r_arr_size; ++i) {
            funcs->set_callback(r_arr, i, funcs->get_callback(arr, mid + 1 + i));
        }
    }

    /* Merge the temp arrays back into `arr[left_start..right_end]`. */
    merge_sort_merge_back(arr, l_arr, l_arr_size, r_arr, r_arr_size, left_start, object, funcs);
}

bool merge_sort(void *arr, uint32_t arr_size, const void *object, const Sort_Funcs *funcs)
{
    void *tmp = funcs->alloc_callback(object, arr_size);

    if (tmp == nullptr) {
        return false;
    }

    // Merge subarrays in bottom up manner.  First merge subarrays of
    // size 1 to create sorted subarrays of size 2, then merge subarrays
    // of size 2 to create sorted subarrays of size 4, and so on.
    for (uint32_t curr_size = 1; curr_size <= arr_size - 1; curr_size = 2 * curr_size) {
        // Pick starting point of different subarrays of current size
        for (uint32_t left_start = 0; left_start < arr_size - 1; left_start += 2 * curr_size) {
            // Find ending point of left subarray. mid+1 is starting
            // point of right
            const uint32_t mid = min_u32(left_start + curr_size - 1, arr_size - 1);
            const uint32_t right_end = min_u32(left_start + 2 * curr_size - 1, arr_size - 1);

            // Merge Subarrays arr[left_start...mid] & arr[mid+1...right_end]
            merge_sort_merge(arr, left_start, mid, right_end, tmp, object, funcs);
        }
    }

    funcs->delete_callback(object, tmp, arr_size);
    return true;
}
