// ou_sdk FrameLink 层测试 — UDP 数据报即帧 + 串口流式切帧 + 模板类型安全收帧
#include "ou/frame_link.hpp"
#include "ou/proto.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <pty.h>
#include <unistd.h>
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
// 测试辅助：与 test_protocol.cpp 一致的已知输入
// ---------------------------------------------------------------------------
static ou::ManualControl makeCtrl() {
    ou::ManualControl m{};
    m.sequence = 7;
    m.x = 600;
    m.y = -300;
    m.z = 125;
    m.p = 0;
    m.r = 0;
    m.yaw = -100;
    return m;
}

static ou::Heartbeat makeHeartbeat() {
    ou::Heartbeat h{};
    h.mode = 1;            // AUTO
    h.system_type = 0x00;  // 机器人 / 机型 0
    h.fw_version = 3;
    h.system_state = 4;    // ARMED
    return h;
}

static bool framesEqual(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

// ---------------------------------------------------------------------------
// UdpFrameLink loopback：send_frame → recv_frame 字节一致，recv_frame_as 解码正确
// ---------------------------------------------------------------------------
static void testUdpLoopback() {
    ou::UdpChannel rx;
    CHECK(rx.bind(18090));

    ou::UdpChannel tx;
    CHECK(tx.bind(18091));
    CHECK(tx.set_peer("127.0.0.1", 18090));

    ou::UdpFrameLink rx_link(rx);
    ou::UdpFrameLink tx_link(tx);

    const auto ctrl = makeCtrl();
    const auto ctrl_frame = ou::encodeManualControl(ctrl);

    CHECK(tx_link.send_frame(ctrl_frame));
    auto raw = rx_link.recv_frame(std::chrono::milliseconds(1000));
    CHECK(raw.has_value());
    if (raw) {
        CHECK(framesEqual(*raw, ctrl_frame));
    }

    // 再发一条，用模板接收
    CHECK(tx_link.send_frame(ctrl_frame));
    auto dec = rx_link.recv_frame_as<ou::ManualControl>(std::chrono::milliseconds(1000));
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->sequence == ctrl.sequence);
        CHECK(dec->x == ctrl.x);
        CHECK(dec->y == ctrl.y);
        CHECK(dec->z == ctrl.z);
        CHECK(dec->p == ctrl.p);
        CHECK(dec->r == ctrl.r);
        CHECK(dec->yaw == ctrl.yaw);
    }
}

// ---------------------------------------------------------------------------
// type 校验：收到 Heartbeat 帧时 recv_frame_as<ManualControl> 返回 nullopt
// ---------------------------------------------------------------------------
static void testUdpTypeMismatch() {
    ou::UdpChannel rx;
    CHECK(rx.bind(18092));

    ou::UdpChannel tx;
    CHECK(tx.bind(18093));
    CHECK(tx.set_peer("127.0.0.1", 18092));

    ou::UdpFrameLink rx_link(rx);
    ou::UdpFrameLink tx_link(tx);

    const auto hb_frame = ou::encodeHeartbeat(makeHeartbeat());

    CHECK(tx_link.send_frame(hb_frame));
    auto dec = rx_link.recv_frame_as<ou::ManualControl>(std::chrono::milliseconds(500));
    CHECK(!dec.has_value());
}

// ---------------------------------------------------------------------------
// UDP 数据报含尾随垃圾 → 不是恰好一帧 → recv_frame 返回 nullopt
// ---------------------------------------------------------------------------
static void testUdpTrailingGarbage() {
    ou::UdpChannel rx;
    CHECK(rx.bind(18094));

    ou::UdpChannel tx;
    CHECK(tx.bind(18095));
    CHECK(tx.set_peer("127.0.0.1", 18094));

    ou::UdpFrameLink rx_link(rx);
    // 这里直接用 UdpChannel 发送脏数据报，因为 tx_link.send_frame 会发整段字节
    const auto ctrl_frame = ou::encodeManualControl(makeCtrl());
    std::vector<uint8_t> garbage = ctrl_frame;
    garbage.push_back(0xFF);
    CHECK(tx.send(garbage));

    auto raw = rx_link.recv_frame(std::chrono::milliseconds(500));
    CHECK(!raw.has_value());
}

// ---------------------------------------------------------------------------
// SerialFrameLink 经 openpty：噪声前缀 + ManualControl 帧 + Heartbeat 帧粘连 → 正确返回 Heartbeat
// ---------------------------------------------------------------------------
static void testSerialNoiseAndStickyFrames() {
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

    ou::SerialFrameLink link(serial);

    const auto hb = makeHeartbeat();
    const auto hb_frame = ou::encodeHeartbeat(hb);
    const auto ctrl_frame = ou::encodeManualControl(makeCtrl());

    std::vector<uint8_t> stream;
    stream.insert(stream.end(), {0xDE, 0xAD});       // 噪声前缀
    stream.insert(stream.end(), ctrl_frame.begin(), ctrl_frame.end());
    stream.insert(stream.end(), hb_frame.begin(), hb_frame.end());

    CHECK(write(master_fd, stream.data(), stream.size()) ==
          static_cast<ssize_t>(stream.size()));

    auto dec = link.recv_frame_as<ou::Heartbeat>(std::chrono::milliseconds(1000));
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->mode == hb.mode);
        CHECK(dec->system_type == hb.system_type);
        CHECK(dec->fw_version == hb.fw_version);
        CHECK(dec->system_state == hb.system_state);
    }

    close(master_fd);
    close(slave_fd);
#endif
}

// ---------------------------------------------------------------------------
// 串口半帧分两批到达 → 仍能组出完整帧
// ---------------------------------------------------------------------------
static void testSerialSplitFrame() {
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

    ou::SerialFrameLink link(serial);

    const auto ctrl = makeCtrl();
    const auto ctrl_frame = ou::encodeManualControl(ctrl);
    const size_t half = ctrl_frame.size() / 2;

    // 先发前半
    CHECK(write(master_fd, ctrl_frame.data(), half) == static_cast<ssize_t>(half));
    auto nothing = link.recv_frame_as<ou::ManualControl>(std::chrono::milliseconds(50));
    CHECK(!nothing.has_value());

    // 再发后半
    CHECK(write(master_fd, ctrl_frame.data() + half, ctrl_frame.size() - half) ==
          static_cast<ssize_t>(ctrl_frame.size() - half));
    auto dec = link.recv_frame_as<ou::ManualControl>(std::chrono::milliseconds(1000));
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->sequence == ctrl.sequence);
        CHECK(dec->x == ctrl.x);
        CHECK(dec->yaw == ctrl.yaw);
    }

    close(master_fd);
    close(slave_fd);
#endif
}

// ---------------------------------------------------------------------------
int main() {
    testUdpLoopback();
    testUdpTypeMismatch();
    testUdpTrailingGarbage();
    testSerialNoiseAndStickyFrames();
    testSerialSplitFrame();

    if (g_failures == 0) {
        std::printf("OK: FrameLink 层测试全部通过\n");
        return 0;
    }
    std::printf("FAIL: %d 个断言失败\n", g_failures);
    return 1;
}
