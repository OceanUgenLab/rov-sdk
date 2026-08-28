// ou_sdk v0.2.0 协议实现 — 显式序列化，固定小端，无 strict-aliasing UB
#include "ou/proto.hpp"

#include <bit>
#include <cstring>
#include <type_traits>

namespace ou {

namespace {

// ---------------------------------------------------------------------------
// 小端字节读写工具
// ---------------------------------------------------------------------------
void putU8(std::vector<uint8_t>& out, uint8_t v) {
    out.push_back(v);
}

void putU16LE(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>(v >> 8));
}

void putF32LE(std::vector<uint8_t>& out, float v) {
    const uint32_t bits = std::bit_cast<uint32_t>(v);
    putU16LE(out, static_cast<uint16_t>(bits & 0xFFFF));
    putU16LE(out, static_cast<uint16_t>(bits >> 16));
}

uint8_t getU8(std::span<const uint8_t> s, size_t off) {
    return s[off];
}

uint16_t getU16LE(std::span<const uint8_t> s, size_t off) {
    return static_cast<uint16_t>(s[off]) |
           static_cast<uint16_t>(static_cast<uint16_t>(s[off + 1]) << 8);
}

float getF32LE(std::span<const uint8_t> s, size_t off) {
    const uint32_t lo = getU16LE(s, off);
    const uint32_t hi = getU16LE(s, off + 2);
    return std::bit_cast<float>(lo | (hi << 16));
}

void putBytes(std::vector<uint8_t>& out, std::span<const uint8_t> bytes) {
    out.insert(out.end(), bytes.begin(), bytes.end());
}

// 组帧公共部分：AA 55 | ver=0x02 | len | type | payload(len) | crc16 LE
std::vector<uint8_t> encodeFrame(std::span<const uint8_t> payload, uint8_t type) {
    std::vector<uint8_t> out;
    out.reserve(kFrameOverhead + payload.size());
    out.push_back(kStx0);
    out.push_back(kStx1);
    out.push_back(kProtocolVersion);
    out.push_back(static_cast<uint8_t>(payload.size()));
    out.push_back(type);
    out.insert(out.end(), payload.begin(), payload.end());
    // CRC 覆盖 ver+len+type+payload，不含 STX
    const uint16_t crc = crc16({out.data() + 2, 3 + payload.size()});
    putU16LE(out, crc);
    return out;
}

// 解码公共校验（不挑 type）：校验 STX/ver/len/type/CRC
bool validateFrameHeader(std::span<const uint8_t> frame, uint8_t expected_type, size_t payload_size) {
    if (frame.size() < kFrameOverhead + payload_size) {
        return false;
    }
    if (frame[0] != kStx0 || frame[1] != kStx1) {
        return false;
    }
    if (frame[2] != kProtocolVersion) {
        return false;
    }
    if (frame[3] != payload_size) {
        return false;
    }
    if (frame[4] != expected_type) {
        return false;
    }
    const uint16_t crc = crc16({frame.data() + 2, 3 + payload_size});
    if (crc != getU16LE(frame, kFrameOverhead + payload_size - 2)) {
        return false;
    }
    return true;
}

}  // namespace

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
// 编码
// ---------------------------------------------------------------------------
std::vector<uint8_t> encodeCmd(const CmdPacket& cmd) {
    std::vector<uint8_t> payload;
    payload.reserve(kCmdPayloadSize);
    putU8(payload, cmd.mode);
    putU8(payload, cmd.armed);
    putBytes(payload, std::span(cmd.reserved));
    putF32LE(payload, cmd.surge);
    putF32LE(payload, cmd.sway);
    putF32LE(payload, cmd.heave);
    putF32LE(payload, cmd.yaw);
    putF32LE(payload, cmd.target_depth);
    putF32LE(payload, cmd.target_heading);
    putF32LE(payload, cmd.target_north);
    putF32LE(payload, cmd.target_east);
    putBytes(payload, std::span(cmd.reserved_tail));
    return encodeFrame(payload, kTypeCmd);
}

std::vector<uint8_t> encodeTele(const TelemetryPacket& tele) {
    std::vector<uint8_t> payload;
    payload.reserve(kTelePayloadSize);
    putF32LE(payload, tele.roll);
    putF32LE(payload, tele.pitch);
    putF32LE(payload, tele.heading);
    putF32LE(payload, tele.yaw_rate);
    putF32LE(payload, tele.depth);
    putF32LE(payload, tele.altitude);
    putF32LE(payload, tele.north);
    putF32LE(payload, tele.east);
    putF32LE(payload, tele.vn);
    putF32LE(payload, tele.ve);
    putF32LE(payload, tele.vd);
    putF32LE(payload, tele.voltage);
    putF32LE(payload, tele.current);
    putF32LE(payload, tele.percent);
    putF32LE(payload, tele.temperature);
    putF32LE(payload, tele.water_temp);
    putF32LE(payload, tele.salinity);
    putF32LE(payload, tele.pressure_bar);
    putF32LE(payload, tele.cable_tension);
    for (size_t i = 0; i < kNumThrusters; ++i) {
        putF32LE(payload, tele.thr[i]);
    }
    putU8(payload, tele.leak);
    putU8(payload, tele.armed);
    putU8(payload, tele.mode);
    putU8(payload, tele.reserved);
    putBytes(payload, std::span(tele.reserved_tail));
    return encodeFrame(payload, kTypeTele);
}

// ---------------------------------------------------------------------------
// 解码
// ---------------------------------------------------------------------------
std::optional<CmdPacket> decodeCmd(std::span<const uint8_t> frame) {
    if (!validateFrameHeader(frame, kTypeCmd, kCmdPayloadSize)) {
        return std::nullopt;
    }
    CmdPacket out{};
    out.mode = getU8(frame, 5 + 0);
    out.armed = getU8(frame, 5 + 1);
    out.reserved[0] = getU8(frame, 5 + 2);
    out.reserved[1] = getU8(frame, 5 + 3);
    out.surge = getF32LE(frame, 5 + 4);
    out.sway = getF32LE(frame, 5 + 8);
    out.heave = getF32LE(frame, 5 + 12);
    out.yaw = getF32LE(frame, 5 + 16);
    out.target_depth = getF32LE(frame, 5 + 20);
    out.target_heading = getF32LE(frame, 5 + 24);
    out.target_north = getF32LE(frame, 5 + 28);
    out.target_east = getF32LE(frame, 5 + 32);
    for (size_t i = 0; i < sizeof(out.reserved_tail); ++i) {
        out.reserved_tail[i] = getU8(frame, 5 + 36 + i);
    }
    return out;
}

std::optional<TelemetryPacket> decodeTele(std::span<const uint8_t> frame) {
    if (!validateFrameHeader(frame, kTypeTele, kTelePayloadSize)) {
        return std::nullopt;
    }
    TelemetryPacket out{};
    out.roll = getF32LE(frame, 5 + 0);
    out.pitch = getF32LE(frame, 5 + 4);
    out.heading = getF32LE(frame, 5 + 8);
    out.yaw_rate = getF32LE(frame, 5 + 12);
    out.depth = getF32LE(frame, 5 + 16);
    out.altitude = getF32LE(frame, 5 + 20);
    out.north = getF32LE(frame, 5 + 24);
    out.east = getF32LE(frame, 5 + 28);
    out.vn = getF32LE(frame, 5 + 32);
    out.ve = getF32LE(frame, 5 + 36);
    out.vd = getF32LE(frame, 5 + 40);
    out.voltage = getF32LE(frame, 5 + 44);
    out.current = getF32LE(frame, 5 + 48);
    out.percent = getF32LE(frame, 5 + 52);
    out.temperature = getF32LE(frame, 5 + 56);
    out.water_temp = getF32LE(frame, 5 + 60);
    out.salinity = getF32LE(frame, 5 + 64);
    out.pressure_bar = getF32LE(frame, 5 + 68);
    out.cable_tension = getF32LE(frame, 5 + 72);
    for (size_t i = 0; i < kNumThrusters; ++i) {
        out.thr[i] = getF32LE(frame, 5 + 76 + 4 * i);
    }
    out.leak = getU8(frame, 5 + 108);
    out.armed = getU8(frame, 5 + 109);
    out.mode = getU8(frame, 5 + 110);
    out.reserved = getU8(frame, 5 + 111);
    for (size_t i = 0; i < sizeof(out.reserved_tail); ++i) {
        out.reserved_tail[i] = getU8(frame, 5 + 112 + i);
    }
    return out;
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
