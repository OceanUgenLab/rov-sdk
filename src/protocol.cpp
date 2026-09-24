// ou_sdk v0.3.0 协议实现 — CRC 与流式切帧
// 各帧的编解码由 codegen 在 generated/ou/protocol.hpp 内联生成。
#include "ou/proto.hpp"

namespace ou {

// ---------------------------------------------------------------------------
// CRC-16/MODBUS
// ---------------------------------------------------------------------------
uint16_t crc16(std::span<const uint8_t> data) {
    uint16_t crc = 0xFFFF;
    for (const uint8_t byte : data) {
        crc ^= byte;
        for (int b = 0; b < 8; ++b) {
            if (crc & 1) {
                crc = static_cast<uint16_t>((crc >> 1) ^ 0xA001);
            } else {
                crc = static_cast<uint16_t>(crc >> 1);
            }
        }
    }
    return crc;
}

// ---------------------------------------------------------------------------
// FrameParser
// ---------------------------------------------------------------------------
void FrameParser::feed(std::span<const uint8_t> bytes) {
    buf_.insert(buf_.end(), bytes.begin(), bytes.end());
}

void FrameParser::reset() {
    buf_.clear();
}

std::optional<std::vector<uint8_t>> FrameParser::next_frame() {
    while (buf_.size() >= kFrameOverhead) {
        // 找 STX
        size_t pos = 0;
        while (pos + 1 < buf_.size() && !(buf_[pos] == kStx0 && buf_[pos + 1] == kStx1)) {
            ++pos;
        }
        if (pos + 1 >= buf_.size()) {
            // 没有完整 STX，保留最后 1 字节
            if (buf_.back() == kStx0) {
                buf_.erase(buf_.begin(), buf_.begin() + pos);
            } else {
                buf_.clear();
            }
            return std::nullopt;
        }

        // 至少有 STX，检查帧头长度
        if (buf_.size() - pos < kFrameOverhead) {
            // 去掉 STX 前的垃圾，保留从 STX 开始的半帧
            buf_.erase(buf_.begin(), buf_.begin() + pos);
            return std::nullopt;
        }

        const uint8_t ver = buf_[pos + 2];
        const uint8_t len = buf_[pos + 3];
        const size_t total = kFrameOverhead + len;

        if (ver != kProtocolVersion) {
            // 版本不对，从 STX 后一字节继续滑窗
            buf_.erase(buf_.begin(), buf_.begin() + pos + 1);
            continue;
        }

        if (buf_.size() - pos < total) {
            // payload 未收齐，保留从 STX 开始
            buf_.erase(buf_.begin(), buf_.begin() + pos);
            return std::nullopt;
        }

        // 计算 CRC：覆盖 ver+len+type+payload
        const uint16_t crc = crc16({buf_.data() + pos + 2, static_cast<size_t>(3) + len});
        const uint16_t got = static_cast<uint16_t>(buf_[pos + total - 2]) |
                             static_cast<uint16_t>(static_cast<uint16_t>(buf_[pos + total - 1]) << 8);

        if (crc != got) {
            // CRC 错，从 STX 后一字节继续滑窗
            buf_.erase(buf_.begin(), buf_.begin() + pos + 1);
            continue;
        }

        // 成功切出一帧
        std::vector<uint8_t> frame(buf_.begin() + pos, buf_.begin() + pos + total);
        buf_.erase(buf_.begin(), buf_.begin() + pos + total);
        return frame;
    }
    return std::nullopt;
}

}  // namespace ou
