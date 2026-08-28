// ou_sdk v0.2.0 协议手写公共头
// 包含 FrameParser、packet_traits、decode 模板与 crc16 声明。
// 结构体/常量/encode 基础声明来自 codegen 产物 <ou/protocol.hpp>。
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

#include <ou/protocol.hpp>  // generated: CmdPacket, TelemetryPacket, constants, encodeCmd/decodeCmd

namespace ou {

// CRC-16/MODBUS (init 0xFFFF, poly 0xA001 反射)
uint16_t crc16(std::span<const uint8_t> data);

// 有状态流式帧切分器：从字节流中提取完整 v0.2.0 帧
class FrameParser {
public:
    FrameParser() = default;

    // 喂入任意字节流片段
    void feed(std::span<const uint8_t> bytes);

    // 丢弃已缓冲的半帧
    void reset();

    // 尝试取出一条完整帧；无完整帧返回 nullopt
    std::optional<std::vector<uint8_t>> next_frame();

private:
    std::vector<uint8_t> buf_;
};

// 类型 → type 字节映射
template <typename Pkt>
struct packet_traits;

template <>
struct packet_traits<CmdPacket> {
    static constexpr uint8_t type = kTypeCmd;
};

template <>
struct packet_traits<TelemetryPacket> {
    static constexpr uint8_t type = kTypeTele;
};

// 类型分发解码
template <typename Pkt>
std::optional<Pkt> decode(std::span<const uint8_t> frame) {
    if constexpr (std::is_same_v<Pkt, CmdPacket>) {
        return decodeCmd(frame);
    } else if constexpr (std::is_same_v<Pkt, TelemetryPacket>) {
        return decodeTele(frame);
    } else {
        static_assert(sizeof(Pkt) == 0, "unsupported packet type for ou::decode");
        return std::nullopt;
    }
}

}  // namespace ou
