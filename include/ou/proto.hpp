// ou_sdk v0.3.0 协议手写公共头
// 包含 FrameParser 与 crc16 声明。
// 各帧结构体/常量/encode/decode 均由 codegen 产物 <ou/protocol.hpp> 内联提供。
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <ou/protocol.hpp>  // generated: frames, encodeX/decodeX, packet_traits

namespace ou {

// CRC-16/MODBUS (init 0xFFFF, poly 0xA001 反射)
uint16_t crc16(std::span<const uint8_t> data);

// 有状态流式帧切分器：从字节流中提取完整 v0.3.0 帧
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

// 从完整帧中取出 type 字节
inline uint8_t frame_type(std::span<const uint8_t> frame) {
    return frame.size() >= 5 ? frame[4] : 0;
}

// 从完整帧中取出 payload 视图（不含帧头与 CRC）
inline std::span<const uint8_t> frame_payload(std::span<const uint8_t> frame) {
    if (frame.size() < kFrameOverhead) {
        return {};
    }
    const size_t len = frame[3];
    if (frame.size() < kFrameOverhead + len) {
        return {};
    }
    return frame.subspan(5, len);
}

// 泛型编解码：按 packet_traits 分发到 codegen 生成的 encodeX/decodeX
template <typename Pkt>
std::vector<uint8_t> encode(const Pkt& pkt) {
    return packet_traits<Pkt>::encode(pkt);
}

template <typename Pkt>
std::optional<Pkt> decode(std::span<const uint8_t> frame) {
    return packet_traits<Pkt>::decode(frame);
}

}  // namespace ou
