// 算力板收遥测最小示例：ou::UdpFrameLink::recv_frame_as<ou::TelemetryPacket> 收帧并打印关键字段
//
// 运行：
//   ./board_telemetry_receiver [监听端口]
//   默认监听 9091。收到 type=0x02 的遥测帧后解码并打印；type 不匹配的帧会被丢弃并继续等待。
#include <ou/protocol.hpp>   // TelemetryPacket（codegen 生成）
#include <ou/frame_link.hpp> // UdpFrameLink、recv_frame_as<Pkt>
#include <ou/udp_channel.hpp>// UdpChannel

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>

int main(int argc, char* argv[]) {
    const uint16_t local_port =
        argc > 1 ? static_cast<uint16_t>(std::stoi(argv[1])) : 9091;

    // 1. 打开本地 UDP 通道并绑定监听端口
    ou::UdpChannel channel;
    if (!channel.bind(local_port)) {
        std::fprintf(stderr, "绑定端口 %u 失败（可能已被占用）\n",
                     static_cast<unsigned>(local_port));
        return 1;
    }

    // 2. 组合层：UdpFrameLink 提供类型安全的模板收帧
    ou::UdpFrameLink link(channel);

    std::printf("监听 0.0.0.0:%u，等待遥测帧（type=0x02）...\n",
                static_cast<unsigned>(local_port));

    // 3. 阻塞接收一条遥测（5 秒超时）；type 不匹配的帧会被自动跳过
    auto tele =
        link.recv_frame_as<ou::TelemetryPacket>(std::chrono::seconds(5));
    if (!tele.has_value()) {
        std::fprintf(stderr, "5 秒内未收到遥测帧\n");
        return 1;
    }

    // 4. 打印关键字段
    std::printf("遥测：深度=%.2f m  航向=%.1f°  横滚=%.1f°  俯仰=%.1f°\n",
                tele->depth, tele->heading, tele->roll, tele->pitch);
    std::printf("电池：%.2f V  %.0f%%  水温=%.1f°C\n", tele->voltage,
                tele->percent, tele->water_temp);
    std::printf("8 路推进器：");
    for (int i = 0; i < 8; ++i) {
        std::printf("%.2f ", tele->thr[i]);
    }
    std::printf("\n");
    std::printf("状态：leak=%u armed=%u mode=%u\n",
                static_cast<unsigned>(tele->leak),
                static_cast<unsigned>(tele->armed),
                static_cast<unsigned>(tele->mode));
    return 0;
}
