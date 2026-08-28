// ou_sdk 通信协议 — 固件二进制帧协议 (C++20 显式序列化实现)
//
// 帧格式: AA 55 | len(1B) | type(1B) | payload(len) | crc16(2B, 小端)
//   - 无 ETX; len 在 type 之前, 1 字节;
//   - CRC 计算自 len 起 (不含 STX): 对应固件 protocol.c:27 `proto_crc16(&out[2], 2+len)`;
//   - 字节序: 协议固定小端 (与 STM32 小端 + 网络字节流约定一致), 显式读写,
//     不依赖宿主平台端序, 无 strict-aliasing UB。
//
// 本头文件仅依赖标准库 (<cstdint>/<span>/<vector>/<optional>), 无 Qt/OpenCV 依赖,
// 可供上位机、算力板、测试工具链任意一方复用。
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ou {

// 帧常量 (与固件 protocol.h / master_bridge.py 逐字节一致)
inline constexpr uint8_t kStx0 = 0xAA;        // 帧头第一字节
inline constexpr uint8_t kStx1 = 0x55;        // 帧头第二字节
inline constexpr uint8_t kTypeCmd = 0x01;     // 控制指令 (主控 -> 从控)
inline constexpr uint8_t kTypeTele = 0x02;    // 遥测 (从控 -> 主控)

// 推进器数量 (与固件 app_config.h 的 NUM_THRUSTERS 一致)
inline constexpr size_t kNumThrusters = 8;

// 帧固定开销 = STX(2) + len(1) + type(1) + crc16(2)
inline constexpr size_t kFrameOverhead = 6;

// 载荷字节数 (与固件 packed 结构体一致)
inline constexpr size_t kCmdPayloadSize = 26;   // 6*float + 2*uint8
inline constexpr size_t kTelePayloadSize = 52;  // 13*float

// 控制指令载荷 (主控 -> 从控). 与固件 CmdPacket 字段顺序逐字节一致.
struct CmdPacket {
    float surge{};      // 前(+) / 后(-)        [-1,1]
    float sway{};       // 右平移(+) / 左平移(-) [-1,1]
    float heave{};      // 上浮(+) / 下潜(-)    [-1,1]
    float yaw{};        // 右转(+) / 左转(-)    [-1,1]
    float servo{};      // 舵机角度 0..180 度
    float arm{};        // 机械臂 0..100 %
    uint8_t light[2]{}; // 照明灯 0..100 %

    friend bool operator==(const CmdPacket&, const CmdPacket&) = default;
};

// 遥测载荷 (从控 -> 主控). 与固件 TelemetryPacket 字段顺序逐字节一致.
struct TelemetryPacket {
    float depth{};                          // 水深 m
    float heading{};                        // 航向 deg
    float roll{};                           // 横滚 deg
    float pitch{};                          // 俯仰 deg
    float battery{};                        // 电池电压 V
    float thr[kNumThrusters]{};             // 各推进器指令百分比 [-1,1]

    friend bool operator==(const TelemetryPacket&, const TelemetryPacket&) = default;
};

// CRC-16/MODBUS (init 0xFFFF, poly 0xA001, 反射) — 与固件 proto_crc16 一致.
// 数据从 len 字节起 (不含 STX), 长度 2+payload_len.
uint16_t crc16(std::span<const uint8_t> data);

// 组帧: 返回完整帧 (AA 55 | len | type | payload | crc16 小端)
std::vector<uint8_t> encodeCmd(const CmdPacket& cmd);
std::vector<uint8_t> encodeTele(const TelemetryPacket& tele);

// 解码: 输入完整帧 (含 STX..CRC), 成功返回载荷; 失败返回 nullopt
// (STX/len/type/crc 任一不匹配均失败). 帧头之外的附加字节会被忽略.
std::optional<CmdPacket> decodeCmd(std::span<const uint8_t> frame);
std::optional<TelemetryPacket> decodeTele(std::span<const uint8_t> frame);

// 从字节流解析出第一条完整 CmdPacket (滑动窗口找 STX + len/type/crc 校验,
// 镜像固件 protocol.c:54-75); 成功则填 out 并返回 true, 否则 false
// (未到齐/CRC 错/类型不符均返回 false)。
bool parseCmdStream(std::span<const uint8_t> buf, CmdPacket& out);

// 帧总长 = kFrameOverhead + payload_len
constexpr size_t frameSize(size_t payload_len) { return kFrameOverhead + payload_len; }

}  // namespace ou
