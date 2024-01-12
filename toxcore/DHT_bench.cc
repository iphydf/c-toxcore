#include "DHT.h"

#include <benchmark/benchmark.h>

#include "DHT_test_util.hh"
#include "crypto_core_test_util.hh"

namespace {

void bench_add_to_list(benchmark::State &state)
{
    Test_Random rng;
    PublicKey const cmp_pk(random_pk(rng));

    for (auto _ : state) {
        // Generate a bunch of other keys, not sorted.
        auto const nodes = vector_of(state.range(0) * 2, random_node_format, rng);

        std::vector<Node_format> node_list(state.range(0));

        // Add all of them.
        for (Node_format const &node : nodes) {
            add_to_list(
                node_list.data(), node_list.size(), node.public_key, &node.ip_port, cmp_pk.data());
        }
    }
}

BENCHMARK(bench_add_to_list)->RangeMultiplier(2)->Range(1, 128);

}
