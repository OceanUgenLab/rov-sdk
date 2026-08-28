// ou_sdk UDP 字节通道
// bind 本地端口；可从首个收到的数据报学习对端地址，也可显式设置对端。
#pragma once

#include "ou/channel.hpp"

#include <string>

namespace ou {

class UdpChannel : public FrameChannel {
public:
    UdpChannel();
    ~UdpChannel() override;

    // 绑定本地端口；失败返回 false（如端口已被占用）
    bool bind(uint16_t local_port);

    // 显式设置对端地址；send 优先发往该地址
    bool set_peer(std::string ip, uint16_t port);

    // 是否已打开并绑定
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
