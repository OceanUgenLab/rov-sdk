// AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ou {

// 帧常量（v0.2.0 固定格式：AA 55 | ver | len | type | payload | crc16）
inline constexpr uint8_t kStx0 = 0xAA;                 // 帧头第一字节
inline constexpr uint8_t kStx1 = 0x55;                 // 帧头第二字节
inline constexpr uint8_t kProtocolVersion = 0x02;  // 协议版本
inline constexpr uint8_t kTypeCmd = 0x01;          // 控制指令（主控 -> 从控）
inline constexpr uint8_t kTypeTele = 0x02;       // 遥测（从控 -> 主控）
inline constexpr size_t kNumThrusters = 8;          // 推进器数量
inline constexpr size_t kFrameOverhead = 7;          // 帧固定开销 = STX(2)+ver(1)+len(1)+type(1)+crc(2)
inline constexpr size_t kCmdPayloadSize = 52;               // 控制指令载荷字节数
inline constexpr size_t kTelePayloadSize = 128;             // 遥测载荷字节数

// 控制指令载荷（主控 -> 从控）
struct CmdPacket {
    uint8_t mode{}; // 运行模式
    uint8_t armed{}; // 解锁标志，1=armed 输出推进器，0=停机
    uint8_t reserved[2]{}; // 保留，发送方清零，接收方忽略
    float surge{}; // 前(+) / 后(-)
    float sway{}; // 右平移(+) / 左平移(-)
    float heave{}; // 上浮(+) / 下潜(-)
    float yaw{}; // 右转(+) / 左转(-)
    float target_depth{}; // 目标深度
    float target_heading{}; // 目标航向
    float target_north{}; // v2, firmware v1 ignores（目标北向位移，寻迹/返航用）
    float target_east{}; // v2, firmware v1 ignores（目标东向位移，寻迹/返航用）
    uint8_t reserved_tail[16]{}; // 扩展预留区（云台/机械臂/照明等后置），发送方清零，接收方忽略

    friend bool operator==(const CmdPacket&, const CmdPacket&) = default;
};

// 遥测载荷（从控 -> 主控）
struct TelemetryPacket {
    float roll{}; // 横滚角
    float pitch{}; // 俯仰角
    float heading{}; // 航向角
    float yaw_rate{}; // 偏航角速度
    float depth{}; // 水深
    float altitude{}; // 距底高度
    float north{}; // 相对原点北向位移
    float east{}; // 相对原点东向位移
    float vn{}; // 北向速度
    float ve{}; // 东向速度
    float vd{}; // 下潜速度（向下为正）
    float voltage{}; // 电池电压
    float current{}; // 电池电流
    float percent{}; // 电池电量百分比
    float temperature{}; // 电池/舱内温度
    float water_temp{}; // 水温
    float salinity{}; // 盐度
    float pressure_bar{}; // 水压
    float cable_tension{}; // 缆绳张力
    float thr[8]{}; // 8 路推进器输出百分比
    uint8_t leak{}; // 漏水检测，1=报警
    uint8_t armed{}; // 当前 armed 状态
    uint8_t mode{}; // 当前运行模式
    uint8_t reserved{}; // 保留，发送方清零，接收方忽略
    uint8_t reserved_tail[16]{}; // 扩展预留区（云台/机械臂/照明等后置），发送方清零，接收方忽略

    friend bool operator==(const TelemetryPacket&, const TelemetryPacket&) = default;
};

// 字段偏移常量（从字段顺序+类型尺寸推导，单位字节）
inline constexpr size_t kCmdPacketModeOffset = 0; // mode
inline constexpr size_t kCmdPacketArmedOffset = 1; // armed
inline constexpr size_t kCmdPacketReservedOffset = 2; // reserved
inline constexpr size_t kCmdPacketSurgeOffset = 4; // surge
inline constexpr size_t kCmdPacketSwayOffset = 8; // sway
inline constexpr size_t kCmdPacketHeaveOffset = 12; // heave
inline constexpr size_t kCmdPacketYawOffset = 16; // yaw
inline constexpr size_t kCmdPacketTargetDepthOffset = 20; // target_depth
inline constexpr size_t kCmdPacketTargetHeadingOffset = 24; // target_heading
inline constexpr size_t kCmdPacketTargetNorthOffset = 28; // target_north
inline constexpr size_t kCmdPacketTargetEastOffset = 32; // target_east
inline constexpr size_t kCmdPacketReservedTailOffset = 36; // reserved_tail
inline constexpr size_t kTelemetryPacketRollOffset = 0; // roll
inline constexpr size_t kTelemetryPacketPitchOffset = 4; // pitch
inline constexpr size_t kTelemetryPacketHeadingOffset = 8; // heading
inline constexpr size_t kTelemetryPacketYawRateOffset = 12; // yaw_rate
inline constexpr size_t kTelemetryPacketDepthOffset = 16; // depth
inline constexpr size_t kTelemetryPacketAltitudeOffset = 20; // altitude
inline constexpr size_t kTelemetryPacketNorthOffset = 24; // north
inline constexpr size_t kTelemetryPacketEastOffset = 28; // east
inline constexpr size_t kTelemetryPacketVnOffset = 32; // vn
inline constexpr size_t kTelemetryPacketVeOffset = 36; // ve
inline constexpr size_t kTelemetryPacketVdOffset = 40; // vd
inline constexpr size_t kTelemetryPacketVoltageOffset = 44; // voltage
inline constexpr size_t kTelemetryPacketCurrentOffset = 48; // current
inline constexpr size_t kTelemetryPacketPercentOffset = 52; // percent
inline constexpr size_t kTelemetryPacketTemperatureOffset = 56; // temperature
inline constexpr size_t kTelemetryPacketWaterTempOffset = 60; // water_temp
inline constexpr size_t kTelemetryPacketSalinityOffset = 64; // salinity
inline constexpr size_t kTelemetryPacketPressureBarOffset = 68; // pressure_bar
inline constexpr size_t kTelemetryPacketCableTensionOffset = 72; // cable_tension
inline constexpr size_t kTelemetryPacketThrOffset = 76; // thr
inline constexpr size_t kTelemetryPacketLeakOffset = 108; // leak
inline constexpr size_t kTelemetryPacketArmedOffset = 109; // armed
inline constexpr size_t kTelemetryPacketModeOffset = 110; // mode
inline constexpr size_t kTelemetryPacketReservedOffset = 111; // reserved
inline constexpr size_t kTelemetryPacketReservedTailOffset = 112; // reserved_tail

// 组帧/解码函数声明（实现在 T4 手写）
std::vector<uint8_t> encodeCmd(const CmdPacket& cmd);
std::vector<uint8_t> encodeTele(const TelemetryPacket& tele);
std::optional<CmdPacket> decodeCmd(std::span<const uint8_t> frame);
std::optional<TelemetryPacket> decodeTele(std::span<const uint8_t> frame);

} // namespace ou