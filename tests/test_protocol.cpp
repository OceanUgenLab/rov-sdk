// ou_sdk 协议 v0.2.0 测试 — golden 帧 + crc + 往返 + FrameParser + packet_traits
#include "ou/proto.hpp"

#include "golden/golden.h"

#include <cstdio>
#include <cstring>
#include <vector>

using ou::CmdPacket;
using ou::TelemetryPacket;

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
static CmdPacket makeCmd() {
    CmdPacket c{};
    c.mode = 1;          // AUTO
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

static TelemetryPacket makeTele() {
    TelemetryPacket t{};
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
    t.mode = 1;          // AUTO
    t.reserved = 0;
    std::memset(t.reserved_tail, 0, sizeof(t.reserved_tail));
    return t;
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
// Golden 编码
// ---------------------------------------------------------------------------
static void testGoldenCmdEncode() {
    const auto frame = ou::encodeCmd(makeCmd());
    CHECK(frame.size() == OU_GOLDEN_CMD_FRAME_LEN);
    CHECK(frame.size() == ou::kFrameOverhead + ou::kCmdPayloadSize);
    CHECK(framesEqual(frame, std::span(OU_GOLDEN_CMD_FRAME, OU_GOLDEN_CMD_FRAME_LEN)));
}

static void testGoldenTeleEncode() {
    const auto frame = ou::encodeTele(makeTele());
    CHECK(frame.size() == OU_GOLDEN_TELE_FRAME_LEN);
    CHECK(frame.size() == ou::kFrameOverhead + ou::kTelePayloadSize);
    CHECK(framesEqual(frame, std::span(OU_GOLDEN_TELE_FRAME, OU_GOLDEN_TELE_FRAME_LEN)));
}

// ---------------------------------------------------------------------------
// 编码 → 解码 往返
// ---------------------------------------------------------------------------
static void testRoundtripCmd() {
    const CmdPacket c = makeCmd();
    const auto frame = ou::encodeCmd(c);
    const auto dec = ou::decodeCmd(frame);
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->mode == c.mode);
        CHECK(dec->armed == c.armed);
        CHECK(dec->surge == c.surge);
        CHECK(dec->sway == c.sway);
        CHECK(dec->heave == c.heave);
        CHECK(dec->yaw == c.yaw);
        CHECK(dec->target_depth == c.target_depth);
        CHECK(dec->target_heading == c.target_heading);
        CHECK(dec->target_north == c.target_north);
        CHECK(dec->target_east == c.target_east);
        CHECK(std::memcmp(dec->reserved_tail, c.reserved_tail, sizeof(c.reserved_tail)) == 0);
    }
}

static void testRoundtripTele() {
    const TelemetryPacket t = makeTele();
    const auto frame = ou::encodeTele(t);
    const auto dec = ou::decodeTele(frame);
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->roll == t.roll);
        CHECK(dec->pitch == t.pitch);
        CHECK(dec->heading == t.heading);
        CHECK(dec->yaw_rate == t.yaw_rate);
        CHECK(dec->depth == t.depth);
        CHECK(dec->altitude == t.altitude);
        CHECK(dec->north == t.north);
        CHECK(dec->east == t.east);
        CHECK(dec->vn == t.vn);
        CHECK(dec->ve == t.ve);
        CHECK(dec->vd == t.vd);
        CHECK(dec->voltage == t.voltage);
        CHECK(dec->current == t.current);
        CHECK(dec->percent == t.percent);
        CHECK(dec->temperature == t.temperature);
        CHECK(dec->water_temp == t.water_temp);
        CHECK(dec->salinity == t.salinity);
        CHECK(dec->pressure_bar == t.pressure_bar);
        CHECK(dec->cable_tension == t.cable_tension);
        for (size_t i = 0; i < ou::kNumThrusters; ++i) {
            CHECK(dec->thr[i] == t.thr[i]);
        }
        CHECK(dec->leak == t.leak);
        CHECK(dec->armed == t.armed);
        CHECK(dec->mode == t.mode);
        CHECK(dec->reserved == t.reserved);
        CHECK(std::memcmp(dec->reserved_tail, t.reserved_tail, sizeof(t.reserved_tail)) == 0);
    }
}

// ---------------------------------------------------------------------------
// 解码错误路径
// ---------------------------------------------------------------------------
static void testDecodeErrorPaths() {
    auto frame = ou::encodeCmd(makeCmd());

    // STX 错误
    {
        auto bad = frame;
        bad[0] = 0x00;
        CHECK(!ou::decodeCmd(bad).has_value());
    }

    // version 错误
    {
        auto bad = frame;
        bad[2] = 0xFF;
        CHECK(!ou::decodeCmd(bad).has_value());
    }

    // len 错误
    {
        auto bad = frame;
        bad[3] = 0xFF;
        CHECK(!ou::decodeCmd(bad).has_value());
    }

    // type 错误
    {
        auto bad = frame;
        bad[4] = ou::kTypeTele;
        CHECK(!ou::decodeCmd(bad).has_value());
        CHECK(!ou::decodeTele(bad).has_value());
    }

    // CRC 损坏
    {
        auto bad = frame;
        bad[bad.size() - 1] ^= 0xFF;
        CHECK(!ou::decodeCmd(bad).has_value());
    }

    // 截断
    {
        auto bad = frame;
        bad.pop_back();
        CHECK(!ou::decodeCmd(bad).has_value());
    }
}

// ---------------------------------------------------------------------------
// 模板类型分发
// ---------------------------------------------------------------------------
static void testPacketTraits() {
    static_assert(ou::packet_traits<CmdPacket>::type == ou::kTypeCmd, "CmdPacket type");
    static_assert(ou::packet_traits<TelemetryPacket>::type == ou::kTypeTele, "TelemetryPacket type");

    const auto cmd_frame = ou::encodeCmd(makeCmd());
    const auto tele_frame = ou::encodeTele(makeTele());

    const auto cmd_dec = ou::decode<CmdPacket>(cmd_frame);
    const auto tele_dec = ou::decode<TelemetryPacket>(tele_frame);
    CHECK(cmd_dec.has_value());
    CHECK(tele_dec.has_value());
    if (cmd_dec) CHECK(cmd_dec->mode == 1);
    if (tele_dec) CHECK(tele_dec->mode == 1);

    // 类型不匹配返回 nullopt
    CHECK(!ou::decode<CmdPacket>(tele_frame).has_value());
    CHECK(!ou::decode<TelemetryPacket>(cmd_frame).has_value());
}

// ---------------------------------------------------------------------------
// FrameParser 流式切帧
// ---------------------------------------------------------------------------
static void testFrameParserNoisePrefix() {
    std::vector<uint8_t> stream;
    stream.insert(stream.end(), {0xDE, 0xAD, 0xBE, 0xEF});  // 噪声前缀
    const auto frame = ou::encodeCmd(makeCmd());
    stream.insert(stream.end(), frame.begin(), frame.end());
    stream.push_back(0xFF);  // 尾部噪声

    ou::FrameParser parser;
    parser.feed(stream);
    const auto got = parser.next_frame();
    CHECK(got.has_value());
    if (got) {
        CHECK(framesEqual(*got, std::span(frame)));
    }
    CHECK(!parser.next_frame().has_value());  // 只剩尾部噪声
}

static void testFrameParserSplitFeed() {
    const auto frame = ou::encodeCmd(makeCmd());

    ou::FrameParser parser;
    // 先喂前半
    parser.feed(std::span(frame.data(), frame.size() / 2));
    CHECK(!parser.next_frame().has_value());

    // 再喂后半
    parser.feed(std::span(frame.data() + frame.size() / 2,
                          frame.size() - frame.size() / 2));
    const auto got = parser.next_frame();
    CHECK(got.has_value());
    if (got) {
        CHECK(framesEqual(*got, std::span(frame)));
    }
    CHECK(!parser.next_frame().has_value());
}

static void testFrameParserBackToBack() {
    const auto cmd = ou::encodeCmd(makeCmd());
    const auto tele = ou::encodeTele(makeTele());

    std::vector<uint8_t> stream;
    stream.insert(stream.end(), cmd.begin(), cmd.end());
    stream.insert(stream.end(), tele.begin(), tele.end());

    ou::FrameParser parser;
    parser.feed(stream);

    const auto f1 = parser.next_frame();
    CHECK(f1.has_value());
    if (f1) CHECK(framesEqual(*f1, std::span(cmd)));

    const auto f2 = parser.next_frame();
    CHECK(f2.has_value());
    if (f2) CHECK(framesEqual(*f2, std::span(tele)));

    CHECK(!parser.next_frame().has_value());
}

static void testFrameParserBadCrcDiscarded() {
    auto bad = ou::encodeCmd(makeCmd());
    bad[bad.size() - 1] ^= 0xFF;  // 损坏 CRC

    std::vector<uint8_t> stream;
    stream.insert(stream.end(), {0x00, 0x11});  // 前缀
    stream.insert(stream.end(), bad.begin(), bad.end());
    const auto good = ou::encodeTele(makeTele());
    stream.insert(stream.end(), good.begin(), good.end());

    ou::FrameParser parser;
    parser.feed(stream);

    // CRC 错的帧被丢弃，继续滑窗应找到后面的 good 帧
    const auto got = parser.next_frame();
    CHECK(got.has_value());
    if (got) {
        CHECK(framesEqual(*got, std::span(good)));
    }
}

static void testFrameParserUnknownTypeStillExtracted() {
    // 构造一条 type=0x03 的合法帧（ver/len/crc 都正确）
    std::vector<uint8_t> frame;
    frame.push_back(ou::kStx0);
    frame.push_back(ou::kStx1);
    frame.push_back(ou::kProtocolVersion);
    frame.push_back(0x02);  // len = 2
    frame.push_back(0x03);  // unknown type
    frame.push_back(0xAB);
    frame.push_back(0xCD);
    const auto crc = ou::crc16(std::span(frame).subspan(2, 3 + 2));
    frame.push_back(static_cast<uint8_t>(crc & 0xFF));
    frame.push_back(static_cast<uint8_t>(crc >> 8));

    ou::FrameParser parser;
    parser.feed(frame);
    const auto got = parser.next_frame();
    CHECK(got.has_value());
    if (got) {
        CHECK(framesEqual(*got, std::span(frame)));
    }
}

static void testFrameParserReset() {
    const auto frame = ou::encodeCmd(makeCmd());

    ou::FrameParser parser;
    parser.feed(std::span(frame.data(), frame.size() / 2));
    parser.reset();
    CHECK(!parser.next_frame().has_value());

    parser.feed(frame);
    const auto got = parser.next_frame();
    CHECK(got.has_value());
    if (got) CHECK(framesEqual(*got, std::span(frame)));
}

// ---------------------------------------------------------------------------
static void testGoldenCmdDecode() {
    const auto dec = ou::decodeCmd(std::span(OU_GOLDEN_CMD_FRAME, OU_GOLDEN_CMD_FRAME_LEN));
    CHECK(dec.has_value());
    if (dec) {
        const CmdPacket c = makeCmd();
        CHECK(dec->mode == c.mode);
        CHECK(dec->armed == c.armed);
        CHECK(dec->surge == c.surge);
        CHECK(dec->sway == c.sway);
        CHECK(dec->heave == c.heave);
        CHECK(dec->yaw == c.yaw);
        CHECK(dec->target_depth == c.target_depth);
        CHECK(dec->target_heading == c.target_heading);
        CHECK(dec->target_north == c.target_north);
        CHECK(dec->target_east == c.target_east);
    }
}

static void testGoldenTeleDecode() {
    const auto dec = ou::decodeTele(std::span(OU_GOLDEN_TELE_FRAME, OU_GOLDEN_TELE_FRAME_LEN));
    CHECK(dec.has_value());
    if (dec) {
        const TelemetryPacket t = makeTele();
        CHECK(dec->roll == t.roll);
        CHECK(dec->pitch == t.pitch);
        CHECK(dec->heading == t.heading);
        CHECK(dec->depth == t.depth);
        CHECK(dec->thr[7] == t.thr[7]);
        CHECK(dec->leak == t.leak);
        CHECK(dec->armed == t.armed);
        CHECK(dec->mode == t.mode);
    }
}

// ---------------------------------------------------------------------------
int main() {
    testCrc16();
    testGoldenCmdEncode();
    testGoldenTeleEncode();
    testRoundtripCmd();
    testRoundtripTele();
    testDecodeErrorPaths();
    testPacketTraits();
    testFrameParserNoisePrefix();
    testFrameParserSplitFeed();
    testFrameParserBackToBack();
    testFrameParserBadCrcDiscarded();
    testFrameParserUnknownTypeStillExtracted();
    testFrameParserReset();
    testGoldenCmdDecode();
    testGoldenTeleDecode();

    if (g_failures == 0) {
        std::printf("OK: 全部 v0.2.0 协议测试通过\n");
        return 0;
    }
    std::printf("FAIL: %d 个断言失败\n", g_failures);
    return 1;
}
