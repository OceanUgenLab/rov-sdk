// ou_sdk FrameLink 组合层实现
#include "ou/frame_link.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace ou {

FrameLink::FrameLink(FrameChannel& channel) : channel_(channel) {}

bool FrameLink::send_frame(std::span<const uint8_t> frame) {
    return channel_.send(frame);
}

std::optional<std::vector<uint8_t>> FrameLink::recv_frame(
    std::chrono::milliseconds timeout) {
    if (timeout.count() <= 0) {
        return std::nullopt;
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true) {
        // 先尝试 parser 中已有缓冲
        auto frame = parser_.next_frame();
        if (frame.has_value()) {
            return frame;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return std::nullopt;
        }
        const auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        if (remaining.count() <= 0) {
            return std::nullopt;
        }

        auto bytes = channel_.recv_bytes(remaining);
        if (!bytes.has_value()) {
            return std::nullopt;
        }
        parser_.feed(*bytes);
    }
}

// -----------------------------------------------------------------------------
// UdpFrameLink：数据报即帧，整包校验
// -----------------------------------------------------------------------------
UdpFrameLink::UdpFrameLink(UdpChannel& channel) : FrameLink(channel) {}

std::optional<std::vector<uint8_t>> UdpFrameLink::recv_frame(
    std::chrono::milliseconds timeout) {
    if (timeout.count() <= 0) {
        return std::nullopt;
    }
    auto bytes = channel_.recv_bytes(timeout);
    if (!bytes.has_value()) {
        return std::nullopt;
    }
    const auto& data = *bytes;

    // 最小长度检查
    if (data.size() < kFrameOverhead) {
        return std::nullopt;
    }

    // UDP 直通语义：数据报必须恰好是一条帧
    const uint8_t len = data[3];
    const size_t total = kFrameOverhead + len;
    if (data.size() != total) {
        return std::nullopt;
    }

    // STX / version 校验
    if (data[0] != kStx0 || data[1] != kStx1 || data[2] != kProtocolVersion) {
        return std::nullopt;
    }

    // CRC 校验（覆盖 ver+len+type+payload，不含 STX）
    const uint16_t crc = crc16({data.data() + 2, static_cast<size_t>(3) + len});
    const uint16_t got = static_cast<uint16_t>(data[total - 2]) |
                         static_cast<uint16_t>(static_cast<uint16_t>(data[total - 1]) << 8);
    if (crc != got) {
        return std::nullopt;
    }

    return data;
}

// -----------------------------------------------------------------------------
// SerialFrameLink：流式切帧由基类 FrameParser 处理
// -----------------------------------------------------------------------------
SerialFrameLink::SerialFrameLink(SerialChannel& channel) : FrameLink(channel) {}

}  // namespace ou
