// ou_sdk 协议 v0.3.0 测试 — golden 帧 + crc + 往返 + FrameParser + packet_traits
#include "ou/proto.hpp"

#include "golden/golden.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

// ---------------------------------------------------------------------------
// 已知输入构造（与 generated/golden/golden.json 一致）
// ---------------------------------------------------------------------------
static ou::Heartbeat makeHeartbeat() {
    ou::Heartbeat h{};
    h.mode = 1;            // AUTO
    h.system_type = 0x00;  // 机器人 / 机型 0
    h.fw_version = 3;
    h.system_state = 4;    // ARMED
    return h;
}

static ou::ManualControl makeManualControl() {
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

static ou::PoseNed makePoseNed() {
    ou::PoseNed p{};
    p.time_boot_ms = 123456;
    p.roll = 0.5f;
    p.pitch = -0.25f;
    p.yaw = 3.0f;
    p.rollspeed = 0.125f;
    p.pitchspeed = -0.125f;
    p.yawspeed = 0.5f;
    p.x = 10.0f;
    p.y = -10.0f;
    p.z = 12.5f;
    p.vx = 0.5f;
    p.vy = -0.5f;
    p.vz = 0.25f;
    return p;
}

static ou::Command makeCommand() {
    ou::Command c{};
    c.command = 1;  // CMD_SET_MODE
    c.param = 3;    // HOLD
    return c;
}

static ou::SysStatus makeSysStatus() {
    ou::SysStatus s{};
    s.sensors_present = 0x0000010F;
    s.sensors_enabled = 0x0000010F;
    s.sensors_health = 0x0000010F;
    s.load = 125;
    s.voltage_total = 29600;
    s.voltage_cell_max = 4200;
    s.voltage_cell_min = 3900;
    s.current_battery = 250;
    s.battery_remaining = 75;
    s.current_consumed = 5000;
    s.battery_temperature = 2500;
    s.battery_fault_bitmask = 0;
    s.drop_rate_comm = 0;
    s.errors_comm = 0;
    s.errors_count[0] = 1;
    s.errors_count[1] = 2;
    s.errors_count[2] = 3;
    s.errors_count[3] = 4;
    s.stream_mask = 5;
    return s;
}

static ou::ParamValue makeParamValue() {
    ou::ParamValue v{};
    const char id[] = "P_GAIN";
    std::memcpy(v.param_id, id, sizeof(id));
    std::memset(v.param_id + sizeof(id), 0, sizeof(v.param_id) - sizeof(id));
    v.param_value = 0.5f;
    v.param_type = 2;
    v.param_count = 128;
    v.param_index = 42;
    return v;
}

static bool framesEqual(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

// ---------------------------------------------------------------------------
// CRC
// ---------------------------------------------------------------------------
static void testCrc16() {
    const uint8_t empty = 0;
    CHECK(ou::crc16({&empty, 0}) == 0xFFFF);

    const std::vector<uint8_t> data = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    CHECK(ou::crc16(data) == OU_GOLDEN_CRC_KNOWN_ANSWER);
    CHECK(ou::crc16(data) == 0x4B37);
}

// ---------------------------------------------------------------------------
// Golden 编码（逐帧比对生成器字节）
// ---------------------------------------------------------------------------
static void testGoldenEncode() {
    CHECK(framesEqual(ou::encodeHeartbeat(makeHeartbeat()),
                      {OU_GOLDEN_HEARTBEAT_FRAME, OU_GOLDEN_HEARTBEAT_FRAME_LEN}));
    CHECK(framesEqual(ou::encodeManualControl(makeManualControl()),
                      {OU_GOLDEN_MANUALCONTROL_FRAME, OU_GOLDEN_MANUALCONTROL_FRAME_LEN}));
    CHECK(framesEqual(ou::encodePoseNed(makePoseNed()),
                      {OU_GOLDEN_POSENED_FRAME, OU_GOLDEN_POSENED_FRAME_LEN}));
    CHECK(framesEqual(ou::encodeCommand(makeCommand()),
                      {OU_GOLDEN_COMMAND_FRAME, OU_GOLDEN_COMMAND_FRAME_LEN}));
    CHECK(framesEqual(ou::encodeSysStatus(makeSysStatus()),
                      {OU_GOLDEN_SYSSTATUS_FRAME, OU_GOLDEN_SYSSTATUS_FRAME_LEN}));
    CHECK(framesEqual(ou::encodeParamValue(makeParamValue()),
                      {OU_GOLDEN_PARAMVALUE_FRAME, OU_GOLDEN_PARAMVALUE_FRAME_LEN}));
}

// ---------------------------------------------------------------------------
// Golden 解码
// ---------------------------------------------------------------------------
static void testGoldenDecode() {
    const auto h = ou::decodeHeartbeat(
        {OU_GOLDEN_HEARTBEAT_FRAME, OU_GOLDEN_HEARTBEAT_FRAME_LEN});
    CHECK(h.has_value());
    if (h) {
        CHECK(h->mode == 1);
        CHECK(h->system_state == 4);
        CHECK(h->fw_version == 3);
    }

    const auto m = ou::decodeManualControl(
        {OU_GOLDEN_MANUALCONTROL_FRAME, OU_GOLDEN_MANUALCONTROL_FRAME_LEN});
    CHECK(m.has_value());
    if (m) {
        CHECK(m->sequence == 7);
        CHECK(m->x == 600);
        CHECK(m->y == -300);
        CHECK(m->z == 125);
        CHECK(m->yaw == -100);
    }

    const auto p = ou::decodePoseNed({OU_GOLDEN_POSENED_FRAME, OU_GOLDEN_POSENED_FRAME_LEN});
    CHECK(p.has_value());
    if (p) {
        CHECK(p->time_boot_ms == 123456);
        CHECK(p->roll == 0.5f);
        CHECK(p->z == 12.5f);
        CHECK(p->vy == -0.5f);
    }

    const auto c = ou::decodeCommand({OU_GOLDEN_COMMAND_FRAME, OU_GOLDEN_COMMAND_FRAME_LEN});
    CHECK(c.has_value());
    if (c) {
        CHECK(c->command == 1);
        CHECK(c->param == 3);
    }

    const auto v = ou::decodeParamValue(
        {OU_GOLDEN_PARAMVALUE_FRAME, OU_GOLDEN_PARAMVALUE_FRAME_LEN});
    CHECK(v.has_value());
    if (v) {
        CHECK(std::memcmp(v->param_id, "P_GAIN", 7) == 0);
        CHECK(v->param_value == 0.5f);
        CHECK(v->param_count == 128);
        CHECK(v->param_index == 42);
    }
}

// ---------------------------------------------------------------------------
// 编解码往返（含 char 数组 / u64 / 定长数组帧）
// ---------------------------------------------------------------------------
static void testRoundTrip() {
    const auto hb = makeHeartbeat();
    CHECK(ou::decodeHeartbeat(ou::encodeHeartbeat(hb)) == hb);

    const auto mc = makeManualControl();
    CHECK(ou::decodeManualControl(ou::encodeManualControl(mc)) == mc);

    const auto pn = makePoseNed();
    CHECK(ou::decodePoseNed(ou::encodePoseNed(pn)) == pn);

    const auto cmd = makeCommand();
    CHECK(ou::decodeCommand(ou::encodeCommand(cmd)) == cmd);

    const auto sys = makeSysStatus();
    CHECK(ou::decodeSysStatus(ou::encodeSysStatus(sys)) == sys);

    const auto pv = makeParamValue();
    CHECK(ou::decodeParamValue(ou::encodeParamValue(pv)) == pv);

    ou::GpsRawInt gps{};
    gps.time_usec = 0x1122334455667788ULL;
    gps.fix_type = 3;
    gps.lat = -1;
    CHECK(ou::decodeGpsRawInt(ou::encodeGpsRawInt(gps)) == gps);

    ou::ServoOutputRaw servo{};
    servo.servo_raw[0] = 1100;
    servo.servo_raw[15] = 1900;
    CHECK(ou::decodeServoOutputRaw(ou::encodeServoOutputRaw(servo)) == servo);

    ou::RcChannels rc{};
    rc.chan_raw[0] = 1500;
    rc.chan_raw[17] = 65535;
    rc.rssi = 87;
    CHECK(ou::decodeRcChannels(ou::encodeRcChannels(rc)) == rc);
}

// ---------------------------------------------------------------------------
// 篡改校验：任一字段破坏 -> 解码失败
// ---------------------------------------------------------------------------
static void testTamperReject() {
    auto frame = ou::encodeManualControl(makeManualControl());

    { // 破坏 STX
        auto bad = frame;
        bad[0] = 0xAB;
        CHECK(!ou::decodeManualControl(bad).has_value());
    }
    { // 破坏 ver
        auto bad = frame;
        bad[2] = 0x02;
        CHECK(!ou::decodeManualControl(bad).has_value());
    }
    { // 破坏 len
        auto bad = frame;
        bad[3] = 13;
        CHECK(!ou::decodeManualControl(bad).has_value());
    }
    { // 破坏 type
        auto bad = frame;
        bad[4] = ou::kTypeCommand;
        CHECK(!ou::decodeManualControl(bad).has_value());
    }
    { // 破坏 payload（CRC 不匹配）
        auto bad = frame;
        bad[5] ^= 0xFF;
        CHECK(!ou::decodeManualControl(bad).has_value());
    }
    { // 破坏 CRC
        auto bad = frame;
        bad.back() ^= 0xFF;
        CHECK(!ou::decodeManualControl(bad).has_value());
    }
    { // 截断
        auto bad = frame;
        bad.pop_back();
        CHECK(!ou::decodeManualControl(bad).has_value());
    }
}

// ---------------------------------------------------------------------------
// packet_traits + 泛型 encode/decode
// ---------------------------------------------------------------------------
static void testTraits() {
    static_assert(ou::packet_traits<ou::Heartbeat>::type == ou::kTypeHeartbeat, "Heartbeat type");
    static_assert(ou::packet_traits<ou::ManualControl>::type == ou::kTypeManualControl,
                  "ManualControl type");
    static_assert(ou::packet_traits<ou::PoseNed>::payload_size == 52, "PoseNed payload size");
    static_assert(ou::packet_traits<ou::Command>::payload_size == 6, "Command payload size");

    const auto hb_frame = ou::encode(makeHeartbeat());
    const auto mc_frame = ou::encode(makeManualControl());
    const auto hb_dec = ou::decode<ou::Heartbeat>(hb_frame);
    const auto mc_dec = ou::decode<ou::ManualControl>(mc_frame);
    CHECK(hb_dec == makeHeartbeat());
    CHECK(mc_dec == makeManualControl());
    CHECK(!ou::decode<ou::Heartbeat>(mc_frame).has_value());
    CHECK(!ou::decode<ou::ManualControl>(hb_frame).has_value());
}

// ---------------------------------------------------------------------------
// 帧级工具：frame_type / frame_payload
// ---------------------------------------------------------------------------
static void testFrameUtils() {
    const auto frame = ou::encodeCommand(makeCommand());
    CHECK(ou::frame_type(frame) == ou::kTypeCommand);
    const auto payload = ou::frame_payload(frame);
    CHECK(payload.size() == ou::kCommandPayloadSize);
    CHECK(payload.size() == 6);
    CHECK(ou::frame_payload({frame.data(), frame.size() - 1}).empty()); // 不完整
}

// ---------------------------------------------------------------------------
// FrameParser：粘包 / 噪声前缀 / 半帧
// ---------------------------------------------------------------------------
static void testFrameParserBasic() {
    const auto a = ou::encodeHeartbeat(makeHeartbeat());
    const auto b = ou::encodePoseNed(makePoseNed());

    std::vector<uint8_t> stream;
    stream.insert(stream.end(), a.begin(), a.end());
    stream.insert(stream.end(), b.begin(), b.end());

    ou::FrameParser parser;
    parser.feed(stream);
    const auto f1 = parser.next_frame();
    const auto f2 = parser.next_frame();
    const auto f3 = parser.next_frame();
    CHECK(f1.has_value() && framesEqual(*f1, a));
    CHECK(f2.has_value() && framesEqual(*f2, b));
    CHECK(!f3.has_value());
}

static void testFrameParserNoiseAndSplit() {
    const auto frame = ou::encodeManualControl(makeManualControl());

    // 噪声前缀 + 帧
    std::vector<uint8_t> noisy = {0x00, 0xAA, 0x13, 0xFF};
    noisy.insert(noisy.end(), frame.begin(), frame.end());

    ou::FrameParser parser;
    parser.feed(noisy);
    const auto f = parser.next_frame();
    CHECK(f.has_value() && framesEqual(*f, frame));

    // 半帧分两次喂
    ou::FrameParser split;
    split.feed({frame.data(), frame.size() / 2});
    CHECK(!split.next_frame().has_value());
    split.feed({frame.data() + frame.size() / 2, frame.size() - frame.size() / 2});
    const auto g = split.next_frame();
    CHECK(g.has_value() && framesEqual(*g, frame));
}

int main() {
    testCrc16();
    testGoldenEncode();
    testGoldenDecode();
    testRoundTrip();
    testTamperReject();
    testTraits();
    testFrameUtils();
    testFrameParserBasic();
    testFrameParserNoiseAndSplit();

    if (g_failures == 0) {
        std::printf("test_protocol: 全部通过\n");
        return 0;
    }
    std::printf("test_protocol: %d 项失败\n", g_failures);
    return 1;
}
