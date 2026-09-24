// 上位机发指令最小示例：ManualControl 杆量 + Command 命令 → encode → UdpFrameLink::send_frame
//
// 运行：
//   ./host_command_sender [对端IP] [对端端口]
//   默认发往 127.0.0.1:9090。发送 10 Hz 手动杆量 2 秒，随后发一条 CMD_SET_MODE(HOLD) 命令。
#include <ou/protocol.hpp>   // ManualControl、Command、encodeX（codegen 生成）
#include <ou/frame_link.hpp> // UdpFrameLink
#include <ou/udp_channel.hpp>// UdpChannel

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>

int main(int argc, char* argv[]) {
    // 目标地址：对端（算力板 / 从控）IP 与端口
    const std::string peer_ip = argc > 1 ? argv[1] : "127.0.0.1";
    const uint16_t peer_port = argc > 2 ? static_cast<uint16_t>(std::stoi(argv[2]))
                                        : 9090;

    // 1. 打开本地 UDP 通道（端口 0 = 系统自动分配，发指令无需固定本地端口）
    ou::UdpChannel channel;
    if (!channel.bind(0)) {
        std::fprintf(stderr, "本地 UDP 绑定失败\n");
        return 1;
    }
    if (!channel.set_peer(peer_ip, peer_port)) {
        std::fprintf(stderr, "设置对端地址失败: %s:%u\n", peer_ip.c_str(),
                     static_cast<unsigned>(peer_port));
        return 1;
    }

    // 2. 组合层：UdpFrameLink 持有 channel，负责「数据报即帧」的整包收发
    ou::UdpFrameLink link(channel);

    // 3. 心跳：上报上位机身份（bit0=1 地面站，SDK 主版本 3）
    ou::Heartbeat hb;
    hb.mode = static_cast<uint8_t>(ou::Mode::kManual);
    hb.system_type = 0x01;
    hb.fw_version = 3;
    hb.system_state = static_cast<uint8_t>(ou::SystemState::kStandby);
    link.send_frame(ou::encodeHeartbeat(hb));

    // 4. 手动杆量：10 Hz × 2 秒，六轴速度指令（0 = 该轴交由飞控自稳）
    for (uint16_t seq = 1; seq <= 20; ++seq) {
        ou::ManualControl ctrl;
        ctrl.sequence = seq;
        ctrl.x = 600;    // 前进 60%
        ctrl.y = 0;
        ctrl.z = 0;      // 深度交给自稳
        ctrl.p = 0;
        ctrl.r = 0;
        ctrl.yaw = -100; // 左偏航 10%
        if (!link.send_frame(ou::encodeManualControl(ctrl))) {
            std::fprintf(stderr, "发送失败\n");
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 5. 命令：切 HOLD 模式（飞控应以 CommandAck 应答，本示例不等待）
    ou::Command cmd;
    cmd.command = static_cast<uint16_t>(ou::CommandId::kCmdSetMode);
    cmd.param = static_cast<uint32_t>(ou::Mode::kHold);
    if (!link.send_frame(ou::encodeCommand(cmd))) {
        std::fprintf(stderr, "发送失败\n");
        return 1;
    }

    std::printf("已发送 20 帧 ManualControl(10Hz) + 1 帧 CMD_SET_MODE(HOLD) -> %s:%u\n",
                peer_ip.c_str(), static_cast<unsigned>(peer_port));
    return 0;
}
