/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2021-2022 The TokTok team.
 */

#include <arpa/inet.h>
#include <sys/socket.h>

#include <algorithm>
#include <cstring>
#include <memory>

#include "../../toxcore/tox_private.h"
#include "fuzz_support.h"

// TODO(iphydf): Put this somewhere shared.
struct Network_Addr {
    struct sockaddr_storage addr;
    size_t size;
};

Network_Base::~Network_Base() { }
Random_Base::~Random_Base() { }

class Fuzz_Network : public Network_Base {
    Fuzz_Data &data_;

    int recv_common(void *buf, size_t buf_len)
    {
        if (data_.size < 2) {
            return -1;
        }

        const size_t fuzz_len = (data_.data[0] << 8) | data_.data[1];
        data_.data += 2;
        data_.size -= 2;

        const size_t res = std::min(buf_len, std::min(fuzz_len, data_.size));

        memcpy(buf, data_.data, res);
        data_.data += res;
        data_.size -= res;

        return res;
    }

public:
    explicit Fuzz_Network(Fuzz_Data &data)
        : data_(data)
    {
    }

    int close(int sock) override { return 0; }
    int accept(int sock) override { return 2; }
    int bind(int sock, const Network_Addr *addr) override { return 0; }
    int listen(int sock, int backlog) override { return 0; }
    int recvbuf(int sock) override
    {
        // TODO(iphydf): Return something sensible here (from the fuzzer): number of
        // bytes to be read from the socket.
        return 0;
    }
    int recv(int sock, uint8_t *buf, size_t len) override
    {
        // Receive data from the fuzzer.
        return recv_common(buf, len);
    }
    int recvfrom(int sock, uint8_t *buf, size_t len, Network_Addr *addr) override
    {
        addr->addr = sockaddr_storage{};
        // Dummy Addr
        addr->addr.ss_family = AF_INET;

        // We want an AF_INET address with dummy values
        sockaddr_in *addr_in = reinterpret_cast<sockaddr_in *>(&addr->addr);
        addr_in->sin_port = 12356;
        addr_in->sin_addr.s_addr = INADDR_LOOPBACK + 1;
        addr->size = sizeof(struct sockaddr);

        return recv_common(buf, len);
    }
    int send(int sock, const uint8_t *buf, size_t len) override
    {
        // Always succeed.
        return static_cast<int>(len);
    }
    int sendto(int sock, const uint8_t *buf, size_t len, const Network_Addr *addr) override
    {
        // Always succeed.
        return static_cast<int>(len);
    }
    int socket(int domain, int type, int proto) override { return 1; }
    int socket_nonblock(int sock, bool nonblock) override { return 0; }
    int getsockopt(int sock, int level, int optname, void *optval, size_t *optlen) override
    {
        memset(optval, 0, *optlen);
        return 0;
    }
    int setsockopt(int sock, int level, int optname, const void *optval, size_t optlen) override
    {
        return 0;
    }

    ~Fuzz_Network() override;
};

Fuzz_Network::~Fuzz_Network() { }

class Fuzz_Random : public Random_Base {
    Fuzz_Data &data_;

public:
    explicit Fuzz_Random(Fuzz_Data &data)
        : data_(data)
    {
    }

    void random_bytes(uint8_t *bytes, size_t length) override
    {
        // Amount of data is limited
        const size_t bytes_read = std::min(length, data_.size);
        // Initialize everything to make MSAN and others happy
        std::memset(bytes, 0, length);
        std::memcpy(bytes, data_.data, bytes_read);
        data_.data += bytes_read;
        data_.size -= bytes_read;
    }
    uint32_t random_uniform(uint32_t upper_bound) override
    {
        uint32_t randnum;
        random_bytes(reinterpret_cast<uint8_t *>(&randnum), sizeof(randnum));
        return randnum % upper_bound;
    }

    virtual ~Fuzz_Random() override;
};

Fuzz_Random::~Fuzz_Random() { }

Fuzz_System::Fuzz_System(Fuzz_Data &input)
    : clock_(0)
    , network_(std::make_unique<Fuzz_Network>(input))
    , ns_{*reinterpret_cast<Network_Funcs **>(network_.get()), network_.get()}
    , random_(std::make_unique<Fuzz_Random>(input))
    , rng_{*reinterpret_cast<Random_Funcs **>(random_.get()), random_.get()}
    , sys{
          [](void *user_data) { return static_cast<Fuzz_System *>(user_data)->clock_; },
          this,
          &rng_,
          &ns_,
      }
{
}
