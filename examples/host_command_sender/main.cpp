// 上位机发指令最小示例：构造 CmdPacket → ou::encodeCmd → ou::UdpFrameLink::send_frame
//
// 运行：
//   ./host_command_sender [对端IP] [对端端口]
//   默认发往 127.0.0.1:9090（可配合 board_telemetry_receiver 对向测试，但两者帧方向不同，
//   此处仅演示编码 + 发送语义，不要求对端真实应答）。
#include <ou/protocol.hpp>   // CmdPacket、encodeCmd（codegen 生成）
#include <ou/frame_link.hpp> // UdpFrameLink
#include <ou/udp_channel.hpp>// UdpChannel

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>

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
    // 显式指定对端地址（否则 send 会等待首个数据报学习对端，此处直接指定）
    if (!channel.set_peer(peer_ip, peer_port)) {
        std::fprintf(stderr, "设置对端地址失败: %s:%u\n", peer_ip.c_str(),
                     static_cast<unsigned>(peer_port));
        return 1;
    }

    // 2. 组合层：UdpFrameLink 持有 channel，负责「数据报即帧」的整包收发
    ou::UdpFrameLink link(channel);

    // 3. 构造控制指令载荷（v0.2.0，payload 52 字节）
    ou::CmdPacket cmd;
    cmd.mode = 0;              // 运行模式：0 = MANUAL 手动遥控
    cmd.armed = 1;             // 解锁：1 = armed 输出推进器
    cmd.surge = 0.5f;          // 前(+) 半速
    cmd.sway = 0.0f;
    cmd.heave = 0.0f;
    cmd.yaw = 0.2f;            // 右转(+) 20%
    cmd.target_depth = 10.0f;  // 目标深度（闭环/自动模式使用）
    cmd.target_heading = 0.0f; // 目标航向

    // 4. 编码为完整帧（AA 55 | ver | len | type | payload | crc16）
    const auto frame = ou::encodeCmd(cmd);

    // 5. 发送整帧（不再二次编码，直接走 channel.send）
    if (!link.send_frame(frame)) {
        std::fprintf(stderr, "发送失败\n");
        return 1;
    }

    std::printf("已发送控制指令帧 %zu 字节 -> %s:%u\n", frame.size(),
                peer_ip.c_str(), static_cast<unsigned>(peer_port));
    std::printf("  surge=%.2f  yaw=%.2f  target_depth=%.1f m  armed=%u\n",
                cmd.surge, cmd.yaw, cmd.target_depth,
                static_cast<unsigned>(cmd.armed));
    return 0;
}
