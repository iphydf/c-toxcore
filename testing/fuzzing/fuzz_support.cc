/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2021-2025 The TokTok team.
 */

#include "fuzz_support.hh"

#ifdef _WIN32
#include <winsock2.h>
// Comment line here to avoid reordering by source code formatters.
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <memory>

#include "../../toxcore/crypto_core.h"
#include "../../toxcore/network.h"
#include "../../toxcore/os_network_impl.h"
#include "../../toxcore/tox_memory_impl.h"
#include "../../toxcore/tox_network_impl.h"
#include "../../toxcore/tox_random_impl.h"
#include "../../toxcore/tox_system_impl.h"
#include "../../toxcore/tox_time_impl.h"
#include "func_conversion.hh"

System::System(std::unique_ptr<Tox_System> in_sys, std::unique_ptr<Tox_Memory> in_mem,
    std::unique_ptr<Tox_Network> in_ns, std::unique_ptr<Tox_Random> in_rng,
    std::unique_ptr<Tox_Time> in_tm)
    : sys(std::move(in_sys))
    , mem(std::move(in_mem))
    , ns(std::move(in_ns))
    , rng(std::move(in_rng))
    , tm(std::move(in_tm))
{
}
System::System(System &&) = default;

System::~System() { }

static int recv_common(Fuzz_Data &input, uint8_t *buf, size_t buf_len)
{
    if (input.size() < 2) {
        errno = ENOMEM;
        return -1;
    }

    CONSUME_OR_ABORT(const uint8_t *fuzz_len_bytes, input, 2);
    const std::size_t fuzz_len = (fuzz_len_bytes[0] << 8) | fuzz_len_bytes[1];

    if (fuzz_len == 0xffff) {
        errno = EWOULDBLOCK;
        if (Fuzz_Data::FUZZ_DEBUG) {
            std::printf("recvfrom: no data for tox1\n");
        }
        return -1;
    }

    if (Fuzz_Data::FUZZ_DEBUG) {
        std::printf(
            "recvfrom: %zu (%02x, %02x) for tox1\n", fuzz_len, input.data()[-2], input.data()[-1]);
    }
    const size_t res = std::min(buf_len, std::min(fuzz_len, input.size()));

    CONSUME_OR_ABORT(const uint8_t *data, input, res);
    std::copy(data, data + res, buf);

    return res;
}

static void *report_alloc(const char *name, const char *func, std::size_t size, void *ptr)
{
    if (Fuzz_Data::FUZZ_DEBUG) {
        printf("%s: %s(%zu): %s\n", name, func, size, ptr == nullptr ? "false" : "true");
    }
    return ptr;
}

template <typename F, F Func, typename... Args>
static void *alloc_common(const char *func, std::size_t size, Fuzz_Data &data, Args... args)
{
    CONSUME1_OR_RETURN_VAL(
        const bool, want_alloc, data, report_alloc("tox1", func, size, Func(args...)));
    if (!want_alloc) {
        return nullptr;
    }
    return report_alloc("tox1", func, size, Func(args...));
}

static constexpr Tox_Memory_Funcs fuzz_memory_funcs = {
    /* .malloc = */
    [](void *v_self, uint32_t size) {
        Fuzz_System *self = static_cast<Fuzz_System *>(v_self);
        return alloc_common<decltype(std::malloc), std::malloc>("malloc", size, self->data, size);
    },
    /* .realloc = */
    [](void *v_self, void *ptr, uint32_t size) {
        Fuzz_System *self = static_cast<Fuzz_System *>(v_self);
        return alloc_common<decltype(std::realloc), std::realloc>(
            "realloc", size, self->data, ptr, size);
    },
    /* .dealloc = */
    [](void *v_self, void *ptr) {
        // Fuzz_System *self = static_cast<Fuzz_System *>(v_self);
        std::free(ptr);
    },
};

static constexpr Tox_Network_Funcs fuzz_network_funcs = {
    /* .close = */ [](void *v_self, Socket sock) { return 0; },
    /* .accept = */ [](void *v_self, Socket sock) { return Socket{1337}; },
    /* .bind = */ [](void *v_self, Socket sock, const IP_Port *addr) { return 0; },
    /* .listen = */ [](void *v_self, Socket sock, int backlog) { return 0; },
    /* .connect = */ [](void *v_self, Socket sock, const IP_Port *addr) { return 0; },
    /* .recvbuf = */
    [](void *v_self, Socket sock) {
        Fuzz_System *self = static_cast<Fuzz_System *>(v_self);
        assert(sock.value == 42 || sock.value == 1337);
        const size_t count = random_u16(self->rng.get());
        return static_cast<int>(std::min(count, self->data.size()));
    },
    /* .recv = */
    [](void *v_self, Socket sock, uint8_t *buf, size_t len) {
        Fuzz_System *self = static_cast<Fuzz_System *>(v_self);
        assert(sock.value == 42 || sock.value == 1337);
        // Receive data from the fuzzer.
        return recv_common(self->data, buf, len);
    },
    /* .recvfrom = */
    [](void *v_self, Socket sock, uint8_t *buf, size_t len, IP_Port *addr) {
        Fuzz_System *self = static_cast<Fuzz_System *>(v_self);
        assert(sock.value == 42 || sock.value == 1337);

        ip_init(&addr->ip, false);
        addr->ip.ip.v4.uint32 = net_htonl(0x7F000002);  // 127.0.0.2
        addr->port = htons(33446);

        return recv_common(self->data, buf, len);
    },
    /* .send = */
    [](void *v_self, Socket sock, const uint8_t *buf, size_t len) {
        assert(sock.value == 42 || sock.value == 1337);
        // Always succeed.
        return static_cast<int>(len);
    },
    /* .sendto = */
    [](void *v_self, Socket sock, const uint8_t *buf, size_t len, const IP_Port *addr) {
        assert(sock.value == 42 || sock.value == 1337);
        // Always succeed.
        return static_cast<int>(len);
    },
    /* .socket = */ [](void *v_self, int domain, int type, int proto) { return Socket{42}; },
    /* .socket_nonblock = */ [](void *v_self, Socket sock, bool nonblock) { return 0; },
    /* .getsockopt = */
    [](void *v_self, Socket sock, int level, int optname, void *optval, size_t *optlen) {
        std::memset(optval, 0, *optlen);
        return 0;
    },
    /* .setsockopt = */
    [](void *v_self, Socket sock, int level, int optname, const void *optval, size_t optlen) {
        return 0;
    },
};

static constexpr Tox_Random_Funcs fuzz_random_funcs = {
    /* .bytes_callback = */
    [](void *v_self, uint8_t *bytes, uint32_t length) {
        Fuzz_System *self = static_cast<Fuzz_System *>(v_self);
        // Initialize the buffer with zeros in case there's no randomness left.
        std::fill_n(bytes, length, 0);

        // For integers, we copy bytes directly, because we want to control the
        // exact values.
        if (length == sizeof(uint8_t) || length == sizeof(uint16_t) || length == sizeof(uint32_t)
            || length == sizeof(uint64_t)) {
            CONSUME_OR_RETURN(const uint8_t *data, self->data, length);
            std::copy(data, data + length, bytes);
            if (Fuzz_Data::FUZZ_DEBUG) {
                if (length == 1) {
                    std::printf("rng: %d (0x%02x)\n", bytes[0], bytes[0]);
                } else {
                    std::printf("rng: %02x..%02x[%u]\n", bytes[0], bytes[length - 1], length);
                }
            }
            return;
        }

        // For nonces and keys, we fill the buffer with the same 1-2 bytes
        // repeated. We only need these to be different enough to not often be
        // the same.
        assert(length == 24 || length == 32);
        // We must cover the case of having only 1 byte left in the input. In
        // that case, we will use the same byte for all the bytes in the output.
        const size_t chunk_size = std::max(self->data.size(), static_cast<std::size_t>(2));
        CONSUME_OR_RETURN(const uint8_t *chunk, self->data, chunk_size);
        if (chunk_size == 2) {
            std::fill_n(bytes, length / 2, chunk[0]);
            std::fill_n(bytes + length / 2, length / 2, chunk[1]);
        } else {
            std::fill_n(bytes, length, chunk[0]);
        }
        if (Fuzz_Data::FUZZ_DEBUG) {
            if (length == 1) {
                std::printf("rng: %d (0x%02x)\n", bytes[0], bytes[0]);
            } else {
                std::printf("rng: %02x..%02x[%u]\n", bytes[0], bytes[length - 1], length);
            }
        }
    },
    /* .uniform_callback = */
    [](void *v_self, uint32_t upper_bound) {
        Fuzz_System *self = static_cast<Fuzz_System *>(v_self);
        uint32_t randnum = 0;
        if (upper_bound > 0) {
            self->rng->funcs->bytes_callback(
                self, reinterpret_cast<uint8_t *>(&randnum), sizeof(randnum));
            randnum %= upper_bound;
        }
        return randnum;
    },
};

static constexpr Tox_Time_Funcs fuzz_time_funcs = {
    /* .monotonic = */
    [](void *v_self) {
        Fuzz_System *self = static_cast<Fuzz_System *>(v_self);
        return self->clock;
    },
};

Fuzz_System::Fuzz_System(Fuzz_Data &input)
    : System{
        std::make_unique<Tox_System>(),
        std::make_unique<Tox_Memory>(Tox_Memory{&fuzz_memory_funcs, this}),
        std::make_unique<Tox_Network>(Tox_Network{&fuzz_network_funcs, this}),
        std::make_unique<Tox_Random>(Tox_Random{&fuzz_random_funcs, this}),
        std::make_unique<Tox_Time>(Tox_Time{&fuzz_time_funcs, this}),
    }
    , data(input)
{
    sys->mem = mem.get();
    sys->ns = ns.get();
    sys->rng = rng.get();
    sys->tm = tm.get();
}

static constexpr Tox_Memory_Funcs null_memory_funcs = {
    /* .malloc = */
    [](void *v_self, uint32_t size) { return std::malloc(size); },
    /* .realloc = */
    [](void *v_self, void *ptr, uint32_t size) { return std::realloc(ptr, size); },
    /* .dealloc = */
    [](void *v_self, void *ptr) { std::free(ptr); },
};

static constexpr Tox_Network_Funcs null_network_funcs = {
    /* .close = */ [](void *v_self, Socket sock) { return 0; },
    /* .accept = */ [](void *v_self, Socket sock) { return Socket{1337}; },
    /* .bind = */ [](void *v_self, Socket sock, const IP_Port *addr) { return 0; },
    /* .listen = */ [](void *v_self, Socket sock, int backlog) { return 0; },
    /* .connect = */ [](void *v_self, Socket sock, const IP_Port *addr) { return 0; },
    /* .recvbuf = */ [](void *v_self, Socket sock) { return 0; },
    /* .recv = */
    [](void *v_self, Socket sock, uint8_t *buf, size_t len) {
        // Always fail.
        errno = ENOMEM;
        return -1;
    },
    /* .recvfrom = */
    [](void *v_self, Socket sock, uint8_t *buf, size_t len, IP_Port *addr) {
        // Always fail.
        errno = ENOMEM;
        return -1;
    },
    /* .send = */
    [](void *v_self, Socket sock, const uint8_t *buf, size_t len) {
        // Always succeed.
        return static_cast<int>(len);
    },
    /* .sendto = */
    [](void *v_self, Socket sock, const uint8_t *buf, size_t len, const IP_Port *addr) {
        // Always succeed.
        return static_cast<int>(len);
    },
    /* .socket = */ [](void *v_self, int domain, int type, int proto) { return Socket{42}; },
    /* .socket_nonblock = */ [](void *v_self, Socket sock, bool nonblock) { return 0; },
    /* .getsockopt = */
    [](void *v_self, Socket sock, int level, int optname, void *optval, size_t *optlen) {
        std::memset(optval, 0, *optlen);
        return 0;
    },
    /* .setsockopt = */
    [](void *v_self, Socket sock, int level, int optname, const void *optval, size_t optlen) {
        return 0;
    },
};

static uint64_t simple_rng(uint64_t &seed)
{
    // https://nuclear.llnl.gov/CNP/rng/rngman/node4.html
    seed = 2862933555777941757LL * seed + 3037000493LL;
    return seed;
}

static constexpr Tox_Random_Funcs null_random_funcs = {
    /* .bytes_callback = */
    [](void *v_self, uint8_t *bytes, uint32_t length) {
        Null_System *self = static_cast<Null_System *>(v_self);
        for (size_t i = 0; i < length; ++i) {
            bytes[i] = simple_rng(self->seed) & 0xff;
        }
    },
    /* .uniform_callback = */
    [](void *v_self, uint32_t upper_bound) {
        Null_System *self = static_cast<Null_System *>(v_self);
        return static_cast<uint32_t>(simple_rng(self->seed)) % upper_bound;
    },
};

Null_System::Null_System()
    : System{
        std::make_unique<Tox_System>(),
        std::make_unique<Tox_Memory>(Tox_Memory{&null_memory_funcs, this}),
        std::make_unique<Tox_Network>(Tox_Network{&null_network_funcs, this}),
        std::make_unique<Tox_Random>(Tox_Random{&null_random_funcs, this}),
        std::make_unique<Tox_Time>(Tox_Time{&fuzz_time_funcs, this}),
    }
{
    sys->mem = mem.get();
    sys->ns = ns.get();
    sys->rng = rng.get();
    sys->tm = tm.get();
}

static constexpr Tox_Memory_Funcs record_memory_funcs = {
    /* .malloc = */
    [](void *v_self, uint32_t size) {
        Record_System *self = static_cast<Record_System *>(v_self);
        self->push(true);
        return report_alloc(self->name_, "malloc", size, std::malloc(size));
    },
    /* .realloc = */
    [](void *v_self, void *ptr, uint32_t size) {
        Record_System *self = static_cast<Record_System *>(v_self);
        self->push(true);
        return report_alloc(self->name_, "realloc", size, std::realloc(ptr, size));
    },
    /* .dealloc = */
    [](void *v_self, void *ptr) {
        // Record_System *self = static_cast<Record_System *>(v_self);
        std::free(ptr);
    },
};

static constexpr Tox_Network_Funcs record_network_funcs = {
    /* .close = */ [](void *v_self, Socket sock) { return 0; },
    /* .accept = */ [](void *v_self, Socket sock) { return Socket{2}; },
    /* .bind = */
    [](void *v_self, Socket sock, const IP_Port *addr) {
        Record_System *self = static_cast<Record_System *>(v_self);
        const uint16_t port = addr->port;
        if (self->global_.bound.find(port) != self->global_.bound.end()) {
            errno = EADDRINUSE;
            return -1;
        }
        self->global_.bound.emplace(port, self);
        self->port = port;
        return 0;
    },
    /* .listen = */ [](void *v_self, Socket sock, int backlog) { return 0; },
    /* .connect = */ [](void *v_self, Socket sock, const IP_Port *addr) { return 0; },
    /* .recvbuf = */ [](void *v_self, Socket sock) { return 0; },
    /* .recv = */
    [](void *v_self, Socket sock, uint8_t *buf, size_t len) {
        // Always fail.
        errno = ENOMEM;
        return -1;
    },
    /* .recvfrom = */
    [](void *v_self, Socket sock, uint8_t *buf, size_t len, IP_Port *addr) {
        Record_System *self = static_cast<Record_System *>(v_self);
        assert(sock.value == 42);
        assert(addr != nullptr);
        if (self->recvq.empty()) {
            self->push("\xff\xff");
            errno = EWOULDBLOCK;
            if (Fuzz_Data::FUZZ_DEBUG) {
                std::printf("%s: recvfrom: no data\n", self->name_);
            }
            return -1;
        }
        const auto [from, packet] = std::move(self->recvq.front());
        self->recvq.pop_front();
        const size_t recvlen = std::min(len, packet.size());
        std::copy(packet.begin(), packet.end(), buf);

        ip_init(&addr->ip, false);
        addr->ip.ip.v4.uint32 = net_htonl(0x7F000002);  // 127.0.0.2
        addr->port = from;

        assert(recvlen > 0 && recvlen <= INT_MAX);
        self->push(uint8_t(recvlen >> 8));
        self->push(uint8_t(recvlen & 0xff));
        if (Fuzz_Data::FUZZ_DEBUG) {
            std::printf("%s: recvfrom: %zu (%02x, %02x)\n", self->name_, recvlen,
                self->recording().end()[-2], self->recording().end()[-1]);
        }
        self->push(buf, recvlen);
        return static_cast<int>(recvlen);
    },
    /* .send = */
    [](void *v_self, Socket sock, const uint8_t *buf, size_t len) {
        // Always succeed.
        return static_cast<int>(len);
    },
    /* .sendto = */
    [](void *v_self, Socket sock, const uint8_t *buf, size_t len,
         const IP_Port *addr) {
        Record_System *self = static_cast<Record_System *>(v_self);
        assert(sock.value == 42);
        auto backend = self->global_.bound.find(addr->port);
        assert(backend != self->global_.bound.end());
        backend->second->receive(self->port, buf, len);
        return static_cast<int>(len);
    },
    /* .socket = */
    [](void *v_self, int domain, int type, int proto) { return Socket{42}; },
    /* .socket_nonblock = */ [](void *v_self, Socket sock, bool nonblock) { return 0; },
    /* .getsockopt = */
    [](void *v_self, Socket sock, int level, int optname, void *optval, size_t *optlen) {
        std::memset(optval, 0, *optlen);
        return 0;
    },
    /* .setsockopt = */
    [](void *v_self, Socket sock, int level, int optname, const void *optval,
         size_t optlen) { return 0; },
};

static constexpr Tox_Random_Funcs record_random_funcs = {
    /* .bytes_callback = */
    [](void *v_self, uint8_t *bytes, uint32_t length) {
        Record_System *self = static_cast<Record_System *>(v_self);
        for (size_t i = 0; i < length; ++i) {
            bytes[i] = simple_rng(self->seed_) & 0xff;
            self->push(bytes[i]);
        }
        if (Fuzz_Data::FUZZ_DEBUG) {
            std::printf(
                "%s: rng: %02x..%02x[%u]\n", self->name_, bytes[0], bytes[length - 1], length);
        }
    },
    /* .uniform_callback = */
    fuzz_random_funcs.uniform_callback,
};

Record_System::Record_System(Global &global, uint64_t seed, const char *name)
    : System{
        std::make_unique<Tox_System>(),
        std::make_unique<Tox_Memory>(Tox_Memory{&record_memory_funcs, this}),
        std::make_unique<Tox_Network>(Tox_Network{&record_network_funcs, this}),
        std::make_unique<Tox_Random>(Tox_Random{&record_random_funcs, this}),
        std::make_unique<Tox_Time>(Tox_Time{&fuzz_time_funcs, this}),
    }
    , global_(global)
    , seed_(seed)
    , name_(name)
{
    sys->mem = mem.get();
    sys->ns = ns.get();
    sys->rng = rng.get();
    sys->tm = tm.get();
}

void Record_System::receive(uint16_t send_port, const uint8_t *buf, size_t len)
{
    assert(port != 0);
    recvq.emplace_back(send_port, std::vector<uint8_t>{buf, buf + len});
}
