// ou_sdk 串口字节通道
// termios 打开串口，纯字节搬运，不组帧。
#pragma once

#include "ou/channel.hpp"

#include <string>

namespace ou {

class SerialChannel : public FrameChannel {
public:
    SerialChannel();
    ~SerialChannel() override;

    // 打开串口设备并配置波特率；失败返回 false
    bool open(std::string device, int baud);

    // 是否已打开
    bool is_open() const;

    bool send(std::span<const uint8_t> data) override;
    std::optional<std::vector<uint8_t>> recv_bytes(
        std::chrono::milliseconds timeout) override;
    std::optional<std::vector<uint8_t>> try_recv_bytes() override;
    int native_handle() const override;

private:
    class Impl;
    Impl *impl_;
};

}  // namespace ou
