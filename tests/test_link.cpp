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
static ou::CmdPacket makeCmd() {
    ou::CmdPacket c{};
    c.mode = 1;
    c.armed = 1;
    c.reserved[0] = 0;
    c.reserved[1] = 0;
    c.surge = 0.5f;
    c.sway = -0.25f;
    c.heave = 1.0f;
    c.yaw = 0.125f;
    c.target_depth = 10.0f;
    c.target_heading = 90.0f;
    c.target_north = 0.0f;
    c.target_east = 0.0f;
    std::memset(c.reserved_tail, 0, sizeof(c.reserved_tail));
    return c;
}

static ou::TelemetryPacket makeTele() {
    ou::TelemetryPacket t{};
    t.roll = 0.0f;
    t.pitch = -0.25f;
    t.heading = 90.0f;
    t.yaw_rate = 0.125f;
    t.depth = 12.5f;
    t.altitude = 5.0f;
    t.north = -0.25f;
    t.east = 0.25f;
    t.vn = 0.5f;
    t.ve = -0.5f;
    t.vd = 0.125f;
    t.voltage = 25.0f;
    t.current = 2.5f;
    t.percent = 75.0f;
    t.temperature = 25.0f;
    t.water_temp = 5.0f;
    t.salinity = 35.0f;
    t.pressure_bar = 10.0f;
    t.cable_tension = 250.0f;
    t.thr[0] = 0.5f;
    t.thr[1] = -0.25f;
    t.thr[2] = 1.0f;
    t.thr[3] = -1.0f;
    t.thr[4] = 0.125f;
    t.thr[5] = -0.5f;
    t.thr[6] = 0.75f;
    t.thr[7] = -0.75f;
    t.leak = 0;
    t.armed = 1;
    t.mode = 1;
    t.reserved = 0;
    std::memset(t.reserved_tail, 0, sizeof(t.reserved_tail));
    return t;
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

    const auto cmd = makeCmd();
    const auto cmd_frame = ou::encodeCmd(cmd);

    CHECK(tx_link.send_frame(cmd_frame));
    auto raw = rx_link.recv_frame(std::chrono::milliseconds(1000));
    CHECK(raw.has_value());
    if (raw) {
        CHECK(framesEqual(*raw, cmd_frame));
    }

    // 再发一条，用模板接收
    CHECK(tx_link.send_frame(cmd_frame));
    auto dec = rx_link.recv_frame_as<ou::CmdPacket>(std::chrono::milliseconds(1000));
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->mode == cmd.mode);
        CHECK(dec->armed == cmd.armed);
        CHECK(dec->surge == cmd.surge);
        CHECK(dec->sway == cmd.sway);
        CHECK(dec->heave == cmd.heave);
        CHECK(dec->yaw == cmd.yaw);
        CHECK(dec->target_depth == cmd.target_depth);
        CHECK(dec->target_heading == cmd.target_heading);
        CHECK(dec->target_north == cmd.target_north);
        CHECK(dec->target_east == cmd.target_east);
    }
}

// ---------------------------------------------------------------------------
// type 校验：收到 Tele 帧时 recv_frame_as<CmdPacket> 返回 nullopt
// ---------------------------------------------------------------------------
static void testUdpTypeMismatch() {
    ou::UdpChannel rx;
    CHECK(rx.bind(18092));

    ou::UdpChannel tx;
    CHECK(tx.bind(18093));
    CHECK(tx.set_peer("127.0.0.1", 18092));

    ou::UdpFrameLink rx_link(rx);
    ou::UdpFrameLink tx_link(tx);

    const auto tele = makeTele();
    const auto tele_frame = ou::encodeTele(tele);

    CHECK(tx_link.send_frame(tele_frame));
    auto dec = rx_link.recv_frame_as<ou::CmdPacket>(std::chrono::milliseconds(500));
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
    const auto cmd_frame = ou::encodeCmd(makeCmd());
    std::vector<uint8_t> garbage = cmd_frame;
    garbage.push_back(0xFF);
    CHECK(tx.send(garbage));

    auto raw = rx_link.recv_frame(std::chrono::milliseconds(500));
    CHECK(!raw.has_value());
}

// ---------------------------------------------------------------------------
// SerialFrameLink 经 openpty：噪声前缀 + Cmd 帧 + Tele 帧粘连 → 正确返回 Tele 结构体
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

    const auto cmd = makeCmd();
    const auto tele = makeTele();
    const auto cmd_frame = ou::encodeCmd(cmd);
    const auto tele_frame = ou::encodeTele(tele);

    std::vector<uint8_t> stream;
    stream.insert(stream.end(), {0xDE, 0xAD});       // 噪声前缀
    stream.insert(stream.end(), cmd_frame.begin(), cmd_frame.end());
    stream.insert(stream.end(), tele_frame.begin(), tele_frame.end());

    CHECK(write(master_fd, stream.data(), stream.size()) ==
          static_cast<ssize_t>(stream.size()));

    auto dec = link.recv_frame_as<ou::TelemetryPacket>(std::chrono::milliseconds(1000));
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->roll == tele.roll);
        CHECK(dec->pitch == tele.pitch);
        CHECK(dec->heading == tele.heading);
        CHECK(dec->depth == tele.depth);
        CHECK(dec->thr[7] == tele.thr[7]);
        CHECK(dec->leak == tele.leak);
        CHECK(dec->armed == tele.armed);
        CHECK(dec->mode == tele.mode);
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

    const auto cmd = makeCmd();
    const auto cmd_frame = ou::encodeCmd(cmd);
    const size_t half = cmd_frame.size() / 2;

    // 先发前半
    CHECK(write(master_fd, cmd_frame.data(), half) == static_cast<ssize_t>(half));
    auto nothing = link.recv_frame_as<ou::CmdPacket>(std::chrono::milliseconds(50));
    CHECK(!nothing.has_value());

    // 再发后半
    CHECK(write(master_fd, cmd_frame.data() + half, cmd_frame.size() - half) ==
          static_cast<ssize_t>(cmd_frame.size() - half));
    auto dec = link.recv_frame_as<ou::CmdPacket>(std::chrono::milliseconds(1000));
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->mode == cmd.mode);
        CHECK(dec->surge == cmd.surge);
        CHECK(dec->target_heading == cmd.target_heading);
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
