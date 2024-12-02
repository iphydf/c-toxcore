/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2023-2024 The TokTok team.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <random>

#include "mem.h"
#include "sort.h"
#include "sort_test_util.hh"
#include "util.h"

namespace {

// A realistic test case where we have a struct with some stuff and an expensive value we compare.
struct Some_Type {
    const Memory *mem;
    std::array<uint32_t, 8> compare_value;
    const char *name;
};

template <typename T, size_t N>
int cmp_uint_array(const std::array<T, N> &a, const std::array<T, N> &b)
{
    for (size_t i = 0; i < a.size(); ++i) {
        const int cmp = cmp_uint(a[i], b[i]);
        if (cmp != 0) {
            return cmp;
        }
    }
    return 0;
}

bool operator<(const Some_Type &a, const Some_Type &b) { return a.compare_value < b.compare_value; }

std::vector<Some_Type> random_vec(benchmark::State &state)
{
    std::mt19937 rng;
    // INT_MAX-1 so later we have room to add 1 larger element if needed.
    std::uniform_int_distribution<uint32_t> dist{
        std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max() - 1};

    std::vector<Some_Type> vec(state.range(0) + 1);
    std::generate(std::begin(vec), std::end(vec), [&]() {
        std::array<uint32_t, 8> compare_value;
        std::generate(
            std::begin(compare_value), std::end(compare_value), [&]() { return dist(rng); });
        return Some_Type{nullptr, compare_value, "hello there"};
    });

    return vec;
}

void BM_merge_sort_with_copy(benchmark::State &state)
{
    auto vec = random_vec(state);

    constexpr auto int_funcs = sort_funcs<Some_Type>(true);

    for (auto _ : state) {
        auto unsorted = vec;
        merge_sort(unsorted.data(), unsorted.size(), &state, &int_funcs);
    }
}

BENCHMARK(BM_merge_sort_with_copy)->RangeMultiplier(2)->Range(8, 8 << 8);

void BM_merge_sort_without_copy(benchmark::State &state)
{
    auto vec = random_vec(state);

    constexpr auto int_funcs = sort_funcs<Some_Type>(false);

    for (auto _ : state) {
        auto unsorted = vec;
        merge_sort(unsorted.data(), unsorted.size(), &state, &int_funcs);
    }
}

BENCHMARK(BM_merge_sort_without_copy)->RangeMultiplier(2)->Range(8, 8 << 8);

void BM_qsort(benchmark::State &state)
{
    auto vec = random_vec(state);

    for (auto _ : state) {
        auto unsorted = vec;
        qsort(unsorted.data(), unsorted.size(), sizeof(unsorted[0]),
            [](const void *va, const void *vb) {
                const auto *a = static_cast<const Some_Type *>(va);
                const auto *b = static_cast<const Some_Type *>(vb);
                return cmp_uint_array(a->compare_value, b->compare_value);
            });
    }
}

BENCHMARK(BM_qsort)->RangeMultiplier(2)->Range(8, 8 << 8);

void BM_std_sort(benchmark::State &state)
{
    auto vec = random_vec(state);

    for (auto _ : state) {
        auto unsorted = vec;
        std::sort(unsorted.begin(), unsorted.end(), [](const Some_Type &a, const Some_Type &b) {
            return a.compare_value < b.compare_value;
        });
    }
}

BENCHMARK(BM_std_sort)->RangeMultiplier(2)->Range(8, 8 << 8);

}

BENCHMARK_MAIN();
