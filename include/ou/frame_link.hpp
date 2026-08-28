// ou_sdk FrameLink 组合层
// 持有 FrameChannel + FrameParser，对外提供 send_frame / recv_frame / recv_frame_as<Pkt>。
#pragma once

#include "ou/channel.hpp"
#include "ou/proto.hpp"
#include "ou/serial_channel.hpp"
#include "ou/udp_channel.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ou {

// FrameLink 基类：基于 FrameChannel 与 FrameParser 的流式帧收发。
// UdpFrameLink 覆盖 recv_frame 以提供「数据报即帧」的直通语义。
class FrameLink {
public:
    explicit FrameLink(FrameChannel& channel);
    virtual ~FrameLink() = default;

    // 发送已编码帧（不再次编码，直接走 channel send）
    bool send_frame(std::span<const uint8_t> frame);

    // 阻塞接收一条完整帧；使用 steady_clock 绝对截止期，超时返回 nullopt。
    // 基类实现基于 FrameParser 的流式切帧，适用于串口等字节流。
    virtual std::optional<std::vector<uint8_t>> recv_frame(
        std::chrono::milliseconds timeout);

    // 模板收帧：在 timeout 内循环 recv_frame，直到收到 type 匹配 packet_traits<Pkt>::type
    // 的帧并通过 ou::decode<Pkt> 解码；type 不匹配则丢弃并继续等待。
    template <typename Pkt>
    std::optional<Pkt> recv_frame_as(std::chrono::milliseconds timeout) {
        if (timeout.count() <= 0) {
            return std::nullopt;
        }
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (true) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return std::nullopt;
            }
            const auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
            if (remaining.count() <= 0) {
                return std::nullopt;
            }
            auto frame = recv_frame(remaining);
            if (!frame.has_value()) {
                return std::nullopt;
            }
            if (frame->size() >= kFrameOverhead + 1 &&
                (*frame)[4] == packet_traits<Pkt>::type) {
                return ou::decode<Pkt>(*frame);
            }
            // type 不匹配：丢弃该帧，继续等待直到超时
        }
    }

protected:
    FrameChannel& channel_;
    FrameParser parser_;
};

// UDP 直通语义：每个数据报必须恰好是一条合法帧（STX + ver + len + CRC）。
class UdpFrameLink : public FrameLink {
public:
    explicit UdpFrameLink(UdpChannel& channel);

    std::optional<std::vector<uint8_t>> recv_frame(
        std::chrono::milliseconds timeout) override;
};

// 串口流式切帧：直接继承基类的 send_frame / recv_frame 行为。
class SerialFrameLink : public FrameLink {
public:
    explicit SerialFrameLink(SerialChannel& channel);
};

}  // namespace ou
