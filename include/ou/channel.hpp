// ou_sdk Channel 抽象基类
// 纯字节搬运通道，不组帧、不解析 STX/CRC。
#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ou {

// 帧字节通道抽象
class FrameChannel {
public:
    virtual ~FrameChannel() = default;

    // 阻塞写，返回是否成功
    virtual bool send(std::span<const uint8_t> data) = 0;

    // 阻塞读，最多等待 timeout；超时或出错返回 nullopt
    virtual std::optional<std::vector<uint8_t>> recv_bytes(
        std::chrono::milliseconds timeout) = 0;

    // 非阻塞读；立即返回，无数据则返回 nullopt
    virtual std::optional<std::vector<uint8_t>> try_recv_bytes() = 0;

    // 返回底层 fd（POSIX）或句柄编号（Windows）
    virtual int native_handle() const = 0;
};

}  // namespace ou
