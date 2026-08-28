// ou_sdk Channel 层测试 — UDP loopback + pty 串口 + 绑定冲突
#include "ou/channel.hpp"
#include "ou/udp_channel.hpp"
#include "ou/serial_channel.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#ifndef _WIN32
#include <pty.h>
#endif

static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

// ---------------------------------------------------------------------------
// UDP loopback + 对端学习
// ---------------------------------------------------------------------------
static void testUdpLoopbackAndPeerLearning() {
    ou::UdpChannel rx;
    CHECK(rx.bind(18080));  // A: bind 本地端口
    CHECK(rx.is_open());

    // B: 显式设置对端为 A
    ou::UdpChannel tx;
    CHECK(tx.bind(18081));
    CHECK(tx.set_peer("127.0.0.1", 18080));

    const std::vector<uint8_t> payload = {0xAA, 0x55, 0x03, 0x01, 0xDE, 0xAD};
    CHECK(tx.send(payload));

    auto got = rx.recv_bytes(std::chrono::milliseconds(1000));
    CHECK(got.has_value());
    if (got) {
        CHECK(got->size() == payload.size());
        CHECK(std::memcmp(got->data(), payload.data(), payload.size()) == 0);
    }

    // A 收到数据报后学习到对端地址；A.send 应到达 B
    const std::vector<uint8_t> reply = {0x01, 0x02, 0x03, 0x04};
    CHECK(rx.send(reply));

    auto back = tx.recv_bytes(std::chrono::milliseconds(1000));
    CHECK(back.has_value());
    if (back) {
        CHECK(back->size() == reply.size());
        CHECK(std::memcmp(back->data(), reply.data(), reply.size()) == 0);
    }
}

// ---------------------------------------------------------------------------
// try_recv_bytes 非阻塞
// ---------------------------------------------------------------------------
static void testUdpTryRecvEmpty() {
    ou::UdpChannel ch;
    CHECK(ch.bind(18082));

    auto t0 = std::chrono::steady_clock::now();
    auto got = ch.try_recv_bytes();
    auto dt = std::chrono::steady_clock::now() - t0;

    CHECK(!got.has_value());
    // 非阻塞应在很短时间内返回（< 50ms）
    CHECK(std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() < 50);
}

// ---------------------------------------------------------------------------
// bind 冲突
// ---------------------------------------------------------------------------
static void testUdpBindConflict() {
    ou::UdpChannel first;
    CHECK(first.bind(18083));

    ou::UdpChannel second;
    CHECK(!second.bind(18083));  // 同一端口应失败
    CHECK(!second.is_open());
    CHECK(second.native_handle() < 0);
}

// ---------------------------------------------------------------------------
// native_handle 返回有效 fd
// ---------------------------------------------------------------------------
static void testUdpNativeHandle() {
    ou::UdpChannel ch;
    CHECK(ch.bind(18084));
    const int fd = ch.native_handle();
    CHECK(fd >= 0);
    CHECK(fcntl(fd, F_GETFD) >= 0);
}

// ---------------------------------------------------------------------------
// SerialChannel 伪终端双向收发
// ---------------------------------------------------------------------------
static void testSerialPtyRoundTrip() {
#ifndef _WIN32
    int master_fd = -1;
    int slave_fd = -1;
    char slave_name[64] = {};

    if (openpty(&master_fd, &slave_fd, slave_name, nullptr, nullptr) != 0) {
        std::perror("openpty");
        CHECK(false);
        return;
    }

    ou::SerialChannel serial;
    CHECK(serial.open(slave_name, 115200));
    CHECK(serial.is_open());
    CHECK(serial.native_handle() >= 0);

    // master -> serial
    const std::vector<uint8_t> to_serial = {0x11, 0x22, 0x33, 0x44, 0x55};
    CHECK(write(master_fd, to_serial.data(), to_serial.size()) ==
          static_cast<ssize_t>(to_serial.size()));

    auto got = serial.recv_bytes(std::chrono::milliseconds(500));
    CHECK(got.has_value());
    if (got) {
        CHECK(got->size() == to_serial.size());
        CHECK(std::memcmp(got->data(), to_serial.data(), to_serial.size()) == 0);
    }

    // serial -> master
    const std::vector<uint8_t> from_serial = {0xAA, 0xBB, 0xCC};
    CHECK(serial.send(from_serial));

    std::vector<uint8_t> buf(from_serial.size());
    ssize_t n = read(master_fd, buf.data(), buf.size());
    CHECK(n == static_cast<ssize_t>(from_serial.size()));
    CHECK(std::memcmp(buf.data(), from_serial.data(), from_serial.size()) == 0);

    close(master_fd);
    close(slave_fd);
#endif
}

// ---------------------------------------------------------------------------
int main() {
    testUdpLoopbackAndPeerLearning();
    testUdpTryRecvEmpty();
    testUdpBindConflict();
    testUdpNativeHandle();
    testSerialPtyRoundTrip();

    if (g_failures == 0) {
        std::printf("OK: Channel 层测试全部通过\n");
        return 0;
    }
    std::printf("FAIL: %d 个断言失败\n", g_failures);
    return 1;
}
