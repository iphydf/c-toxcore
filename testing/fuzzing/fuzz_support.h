/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2021-2022 The TokTok team.
 */

#ifndef C_TOXCORE_TESTING_FUZZING_FUZZ_SUPPORT_H
#define C_TOXCORE_TESTING_FUZZING_FUZZ_SUPPORT_H

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>

#include "../../toxcore/crypto_core.h"
#include "../../toxcore/network.h"
#include "../../toxcore/tox.h"
#include "../../toxcore/tox_private.h"

struct Fuzz_Data {
    const uint8_t *data;
    std::size_t size;

    Fuzz_Data(const uint8_t *input_data, std::size_t input_size)
        : data(input_data), size(input_size)
    {}

    Fuzz_Data &operator=(const Fuzz_Data &rhs) = delete;
    Fuzz_Data(const Fuzz_Data &rhs) = delete;

    uint8_t consume1()
    {
        const uint8_t val = data[0];
        ++data;
        --size;
        return val;
    }

    const uint8_t *consume(std::size_t count)
    {
        const uint8_t *val = data;
        data += count;
        size -= count;
        return val;
    }
};

/** @brief Consumes 1 byte of the fuzzer input or returns if no data available.
 *
 * This advances the fuzzer input data by 1 byte and consumes that byte in the
 * declaration.
 *
 * @example
 * @code
 * CONSUME1_OR_RETURN(const uint8_t one_byte, input);
 * @endcode
 */
#define CONSUME1_OR_RETURN(DECL, INPUT) \
    if (INPUT.size < 1) {               \
        return;                         \
    }                                   \
    DECL = INPUT.consume1()

/** @brief Consumes SIZE bytes of the fuzzer input or returns if not enough data available.
 *
 * This advances the fuzzer input data by SIZE byte and consumes those bytes in
 * the declaration. If less than SIZE bytes are available in the fuzzer input,
 * this macro returns from the enclosing function.
 *
 * @example
 * @code
 * CONSUME_OR_RETURN(const uint8_t *ten_bytes, input, 10);
 * @endcode
 */
#define CONSUME_OR_RETURN(DECL, INPUT, SIZE) \
    if (INPUT.size < SIZE) {                 \
        return;                              \
    }                                        \
    DECL = INPUT.consume(SIZE)

inline void fuzz_select_target(uint8_t selector, Fuzz_Data &input)
{
    // The selector selected no function, so we do nothing and rely on the
    // fuzzer to come up with a better selector.
}

template <typename Arg, typename... Args>
void fuzz_select_target(uint8_t selector, Fuzz_Data &input, Arg &&fn, Args &&... args)
{
    if (selector == sizeof...(Args)) {
        return fn(input);
    }
    return fuzz_select_target(selector - 1, input, std::forward<Args>(args)...);
}

template <typename... Args>
void fuzz_select_target(const uint8_t *data, std::size_t size, Args &&... args)
{
    Fuzz_Data input{data, size};

    CONSUME1_OR_RETURN(uint8_t selector, input);
    return fuzz_select_target(selector, input, std::forward<Args>(args)...);
}

class Network_Base {
public:
    virtual int close(int sock) = 0;
    virtual int accept(int sock) = 0;
    virtual int bind(int sock, const Network_Addr *addr) = 0;
    virtual int listen(int sock, int backlog) = 0;
    virtual int recvbuf(int sock) = 0;
    virtual int recv(int sock, uint8_t *buf, size_t len) = 0;
    virtual int recvfrom(int sock, uint8_t *buf, size_t len, Network_Addr *addr) = 0;
    virtual int send(int sock, const uint8_t *buf, size_t len) = 0;
    virtual int sendto(int sock, const uint8_t *buf, size_t len, const Network_Addr *addr) = 0;
    virtual int socket(int domain, int type, int proto) = 0;
    virtual int socket_nonblock(int sock, bool nonblock) = 0;
    virtual int getsockopt(int sock, int level, int optname, void *optval, size_t *optlen) = 0;
    virtual int setsockopt(int sock, int level, int optname, const void *optval, size_t optlen) = 0;

    virtual ~Network_Base();
};

class Random_Base {
public:
    virtual void random_bytes(uint8_t *bytes, size_t length) = 0;
    virtual uint32_t random_uniform(uint32_t upper_bound) = 0;

    virtual ~Random_Base();
};

class Fuzz_System {
    uint64_t clock_;

    std::unique_ptr<Network_Base> network_;
    const Network ns_;

    std::unique_ptr<Random_Base> random_;
    const Random rng_;

public:
    const Tox_System sys;

    Fuzz_System(Fuzz_Data &input);

    void advance_clock(uint64_t amount) {
        clock_ += amount;
    }

    // Non-copyable because sys contains pointers to rng and ns.
    Fuzz_System(const Fuzz_System &rhs) = delete;
    Fuzz_System &operator=(const Fuzz_System &rhs) = delete;
};

#endif  // C_TOXCORE_TESTING_FUZZING_FUZZ_SUPPORT_H
