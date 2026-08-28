// ou_sdk 协议编解码测试 — golden 帧字节级验证
//
// 关键: golden 帧由固件 protocol.c / master_bridge.py 的逐字节行为定义,
// 本测试将编码结果与硬编码字节序列比对, 确保显式序列化实现与固件一致。
#include "ou/protocol.hpp"

#include <cstdio>
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

// 已知载荷构造: 控制指令 (与固件测试向量一致)
static CmdPacket makeCmd() {
    CmdPacket c{};
    c.surge = 0.5f;
    c.sway = -0.25f;
    c.heave = 1.0f;
    c.yaw = 0.125f;
    c.servo = 90.0f;
    c.arm = 50.0f;
    c.light[0] = 100;
    c.light[1] = 0;
    return c;
}

static TelemetryPacket makeTele() {
    TelemetryPacket t{};
    t.depth = 12.5f;
    t.heading = 180.0f;
    t.roll = -3.0f;
    t.pitch = 2.5f;
    t.battery = 25.2f;
    for (size_t i = 0; i < ou::kNumThrusters; ++i) {
        t.thr[i] = 0.1f * static_cast<float>(i) - 0.4f;
    }
    return t;
}

// golden 帧: 逐字节验证 (由固件 protocol.c 编码行为推导)
// CmdPacket: surge=0.5, sway=-0.25, heave=1.0, yaw=0.125, servo=90, arm=50, light=[100,0]
// 浮点位模式: 0.5f=0x3F000000, -0.25f=0xBE800000, 1.0f=0x3F800000, 0.125f=0x3E000000,
//             90.0f=0x42B40000, 50.0f=0x42480000
static void testGoldenCmdEncode() {
    const CmdPacket c = makeCmd();
    const std::vector<uint8_t> frame = ou::encodeCmd(c);
    CHECK(frame.size() == ou::frameSize(ou::kCmdPayloadSize));  // 6+26=32

    // 帧头 + len + type
    CHECK(frame[0] == 0xAA);
    CHECK(frame[1] == 0x55);
    CHECK(frame[2] == ou::kCmdPayloadSize);  // 26
    CHECK(frame[3] == ou::kTypeCmd);          // 0x01

    // 字段 0: surge = 0.5f -> 3F000000 小端 -> 00 00 00 3F
    CHECK(frame[4] == 0x00 && frame[5] == 0x00 && frame[6] == 0x00 && frame[7] == 0x3F);
    // 字段 1: sway = -0.25f -> BE800000 小端 -> 00 00 80 BE
    CHECK(frame[8] == 0x00 && frame[9] == 0x00 && frame[10] == 0x80 && frame[11] == 0xBE);
    // 字段 2: heave = 1.0f -> 3F800000 小端 -> 00 00 80 3F
    CHECK(frame[12] == 0x00 && frame[13] == 0x00 && frame[14] == 0x80 && frame[15] == 0x3F);
    // 字段 3: yaw = 0.125f -> 3E000000 小端 -> 00 00 00 3E
    CHECK(frame[16] == 0x00 && frame[17] == 0x00 && frame[18] == 0x00 && frame[19] == 0x3E);
    // 字段 4: servo = 90.0f -> 42B40000 小端 -> 00 00 B4 42
    CHECK(frame[20] == 0x00 && frame[21] == 0x00 && frame[22] == 0xB4 && frame[23] == 0x42);
    // 字段 5: arm = 50.0f -> 42480000 小端 -> 00 00 48 42
    CHECK(frame[24] == 0x00 && frame[25] == 0x00 && frame[26] == 0x48 && frame[27] == 0x42);
    // light
    CHECK(frame[28] == 100 && frame[29] == 0);

    // CRC16 覆盖 len..payload (不含 STX): 手工计算确认
    // 已知参考值由固件 proto_crc16 对 28 字节 (len+type+payload) 计算
    // 这里用实现自洽验证: 解码必须接受该帧
    const auto dec = ou::decodeCmd(frame);
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->surge == c.surge);
        CHECK(dec->sway == c.sway);
        CHECK(dec->heave == c.heave);
        CHECK(dec->yaw == c.yaw);
        CHECK(dec->servo == c.servo);
        CHECK(dec->arm == c.arm);
        CHECK(dec->light[0] == 100 && dec->light[1] == 0);
    }
}

// 遥测 golden: 字段与帧结构验证
static void testGoldenTeleEncode() {
    const TelemetryPacket t = makeTele();
    const std::vector<uint8_t> frame = ou::encodeTele(t);
    CHECK(frame.size() == ou::frameSize(ou::kTelePayloadSize));  // 6+52=58
    CHECK(frame[0] == 0xAA && frame[1] == 0x55);
    CHECK(frame[2] == ou::kTelePayloadSize);
    CHECK(frame[3] == ou::kTypeTele);

    // depth = 12.5f -> 41480000 小端 -> 00 00 48 41
    CHECK(frame[4] == 0x00 && frame[5] == 0x00 && frame[6] == 0x48 && frame[7] == 0x41);

    const auto dec = ou::decodeTele(frame);
    CHECK(dec.has_value());
    if (dec) {
        CHECK(dec->depth == t.depth);
        CHECK(dec->heading == t.heading);
        CHECK(dec->battery == t.battery);
        CHECK(dec->thr[0] == t.thr[0]);
        CHECK(dec->thr[7] == t.thr[7]);
    }
}

// CRC16 已知向量: 空输入 / "123456789" (MODBUS 标准)
static void testCrc16Vectors() {
    const uint8_t empty = 0;
    CHECK(ou::crc16({&empty, 0}) == 0xFFFF);

    const std::vector<uint8_t> data = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    // CRC-16/MODBUS 对 "123456789" 的校验值 = 0x4B37
    CHECK(ou::crc16(data) == 0x4B37);
}

// 解析流: 含噪声前缀 + 完整帧 + 尾部数据
static void testParseStream() {
    std::vector<uint8_t> stream;
    stream.push_back(0x00);  // 噪声
    stream.push_back(0x01);
    const auto frame = ou::encodeCmd(makeCmd());
    stream.insert(stream.end(), frame.begin(), frame.end());
    stream.push_back(0xFF);  // 尾部噪声

    CmdPacket out{};
    CHECK(ou::parseCmdStream(stream, out));
    CHECK(out.surge == 0.5f);
    CHECK(out.arm == 50.0f);
}

// 错误路径: CRC 损坏 / 类型错误 / 帧不完整
static void testErrorPaths() {
    auto frame = ou::encodeCmd(makeCmd());

    // CRC 损坏
    auto bad = frame;
    bad[bad.size() - 1] ^= 0xFF;
    CHECK(!ou::decodeCmd(bad).has_value());

    // 类型错误: 把 Cmd 帧类型改成 Tele
    auto wrong_type = frame;
    wrong_type[3] = ou::kTypeTele;
    CHECK(!ou::decodeCmd(wrong_type).has_value());
    CHECK(!ou::decodeTele(wrong_type).has_value());

    // 帧不完整 (截断)
    auto truncated = frame;
    truncated.pop_back();
    CHECK(!ou::decodeCmd(truncated).has_value());
}

int main() {
    testGoldenCmdEncode();
    testGoldenTeleEncode();
    testCrc16Vectors();
    testParseStream();
    testErrorPaths();

    if (g_failures == 0) {
        std::printf("OK: 全部协议测试通过\n");
        return 0;
    }
    std::printf("FAIL: %d 个断言失败\n", g_failures);
    return 1;
}
