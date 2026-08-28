// ou_sdk 通信协议实现 — 显式序列化, 固定小端, 无 strict-aliasing UB
#include "ou/protocol.hpp"

#include <bit>
#include <cstring>

namespace ou {

// ---------------------------------------------------------------------------
// 小端字节读写工具 (协议固定小端, 与固件 STM32 一致)
// ---------------------------------------------------------------------------
namespace {

void putU16LE(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>(v >> 8));
}

void putF32LE(std::vector<uint8_t>& out, float v) {
    // std::bit_cast: 严格定义 float 位模式到 uint32 的转换, 无 UB
    const uint32_t bits = std::bit_cast<uint32_t>(v);
    putU16LE(out, static_cast<uint16_t>(bits & 0xFFFF));
    putU16LE(out, static_cast<uint16_t>(bits >> 16));
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

// 组帧公共部分: AA 55 | len | type | payload(len) | crc16(2B 小端)
std::vector<uint8_t> encodePacket(std::span<const uint8_t> payload, uint8_t type) {
    std::vector<uint8_t> out;
    out.reserve(kFrameOverhead + payload.size());
    out.push_back(kStx0);
    out.push_back(kStx1);
    out.push_back(static_cast<uint8_t>(payload.size()));
    out.push_back(type);
    out.insert(out.end(), payload.begin(), payload.end());
    // 与固件 protocol.c:27 一致: CRC 从 len 起算 (不含 STX)
    const uint16_t crc = crc16({out.data() + 2, 2 + payload.size()});
    putU16LE(out, crc);
    return out;
}

void writeCmdPayload(std::vector<uint8_t>& out, const CmdPacket& cmd) {
    out.reserve(out.size() + kCmdPayloadSize);
    putF32LE(out, cmd.surge);
    putF32LE(out, cmd.sway);
    putF32LE(out, cmd.heave);
    putF32LE(out, cmd.yaw);
    putF32LE(out, cmd.servo);
    putF32LE(out, cmd.arm);
    out.push_back(cmd.light[0]);
    out.push_back(cmd.light[1]);
}

void writeTelePayload(std::vector<uint8_t>& out, const TelemetryPacket& tele) {
    out.reserve(out.size() + kTelePayloadSize);
    putF32LE(out, tele.depth);
    putF32LE(out, tele.heading);
    putF32LE(out, tele.roll);
    putF32LE(out, tele.pitch);
    putF32LE(out, tele.battery);
    for (size_t i = 0; i < kNumThrusters; ++i) {
        putF32LE(out, tele.thr[i]);
    }
}

template <typename T>
bool readPayload(std::span<const uint8_t> frame, T& out, size_t payload_size) {
    // 校验: 帧长 = 4 头 + payload + 2 crc; STX/type/len/crc 全部匹配
    const size_t total = 4 + payload_size + 2;
    if (frame.size() < total) {
        return false;
    }
    if (frame[0] != kStx0 || frame[1] != kStx1) {
        return false;
    }
    const uint8_t type = frame[3];
    const bool type_ok =
        (std::is_same_v<T, CmdPacket> && type == kTypeCmd) ||
        (std::is_same_v<T, TelemetryPacket> && type == kTypeTele);
    if (!type_ok || frame[2] != payload_size) {
        return false;
    }
    const uint16_t crc = crc16(frame.subspan(2, 2 + payload_size));
    if (crc != getU16LE(frame, 4 + payload_size)) {
        return false;
    }
    // 显式字段解析 (固定小端)
    if constexpr (std::is_same_v<T, CmdPacket>) {
        out.surge = getF32LE(frame, 4 + 0);
        out.sway = getF32LE(frame, 4 + 4);
        out.heave = getF32LE(frame, 4 + 8);
        out.yaw = getF32LE(frame, 4 + 12);
        out.servo = getF32LE(frame, 4 + 16);
        out.arm = getF32LE(frame, 4 + 20);
        out.light[0] = frame[4 + 24];
        out.light[1] = frame[4 + 25];
    } else {
        out.depth = getF32LE(frame, 4 + 0);
        out.heading = getF32LE(frame, 4 + 4);
        out.roll = getF32LE(frame, 4 + 8);
        out.pitch = getF32LE(frame, 4 + 12);
        out.battery = getF32LE(frame, 4 + 16);
        for (size_t i = 0; i < kNumThrusters; ++i) {
            out.thr[i] = getF32LE(frame, 4 + 20 + 4 * i);
        }
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// 公开 API
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

std::vector<uint8_t> encodeCmd(const CmdPacket& cmd) {
    std::vector<uint8_t> payload;
    writeCmdPayload(payload, cmd);
    return encodePacket(payload, kTypeCmd);
}

std::vector<uint8_t> encodeTele(const TelemetryPacket& tele) {
    std::vector<uint8_t> payload;
    writeTelePayload(payload, tele);
    return encodePacket(payload, kTypeTele);
}

std::optional<CmdPacket> decodeCmd(std::span<const uint8_t> frame) {
    CmdPacket out;
    if (readPayload(frame, out, kCmdPayloadSize)) {
        return out;
    }
    return std::nullopt;
}

std::optional<TelemetryPacket> decodeTele(std::span<const uint8_t> frame) {
    TelemetryPacket out;
    if (readPayload(frame, out, kTelePayloadSize)) {
        return out;
    }
    return std::nullopt;
}

bool parseCmdStream(std::span<const uint8_t> buf, CmdPacket& out) {
    size_t i = 0;
    while (i + kFrameOverhead <= buf.size()) {
        if (buf[i] == kStx0 && buf[i + 1] == kStx1) {
            const uint8_t plen = buf[i + 2];
            const uint8_t type = buf[i + 3];
            const size_t total = 4 + plen + 2;
            if (i + total > buf.size()) {
                return false;  // 数据尚未到齐 (对应固件 protocol.c:62)
            }
            const uint16_t crc = crc16(buf.subspan(i + 2, 2 + plen));
            const uint16_t got = getU16LE(buf, i + 4 + plen);
            if (type == kTypeCmd && plen == kCmdPayloadSize && crc == got) {
                readPayload(buf.subspan(i), out, kCmdPayloadSize);
                return true;
            }
            i += total;  // 帧无效, 跳过整帧 (对应固件 protocol.c:69)
        } else {
            ++i;
        }
    }
    return false;
}

}  // namespace ou
