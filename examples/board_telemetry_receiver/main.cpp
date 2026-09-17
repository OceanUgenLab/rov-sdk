// 算力板收遥测最小示例：UdpFrameLink::recv_frame + 按帧 type 分发解码打印
//
// 运行：
//   ./board_telemetry_receiver [监听端口]
//   默认监听 9091。循环接收，按 type 解码 Heartbeat / PoseNed / WaterDepth / SysStatus；
// type 不匹配的帧跳过，Ctrl+C 退出。
#include <ou/proto.hpp>       // frame_type、decode<Pkt>
#include <ou/frame_link.hpp>  // UdpFrameLink
#include <ou/protocol.hpp>    // 各帧（codegen 生成）
#include <ou/udp_channel.hpp> // UdpChannel

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

    // 2. 组合层
    ou::UdpFrameLink link(channel);
    std::printf("监听 0.0.0.0:%u，循环接收遥测帧...\n",
                static_cast<unsigned>(local_port));

    while (true) {
        // 3. 收一条完整帧（5 秒超时）
        auto frame = link.recv_frame(std::chrono::seconds(5));
        if (!frame.has_value()) {
            std::printf("5 秒未收到帧，继续等待\n");
            continue;
        }

        // 4. 按 type 分发解码
        switch (ou::frame_type(*frame)) {
        case ou::kTypeHeartbeat: {
            const auto hb = ou::decode<ou::Heartbeat>(*frame);
            if (hb) {
                std::printf("[心跳] mode=%u state=%u fw=%u type=0x%02X\n",
                            static_cast<unsigned>(hb->mode),
                            static_cast<unsigned>(hb->system_state),
                            static_cast<unsigned>(hb->fw_version),
                            static_cast<unsigned>(hb->system_type));
            }
            break;
        }
        case ou::kTypePoseNed: {
            const auto p = ou::decode<ou::PoseNed>(*frame);
            if (p) {
                std::printf("[位姿] roll=%.2f pitch=%.2f yaw=%.2f z=%.2f vx=%.2f\n",
                            p->roll, p->pitch, p->yaw, p->z, p->vx);
            }
            break;
        }
        case ou::kTypeWaterDepth: {
            const auto w = ou::decode<ou::WaterDepth>(*frame);
            if (w) {
                std::printf("[水深] bottom=%.2f m altitude=%.2f m temp=%.1f°C healthy=%u\n",
                            w->bottom_distance, w->altitude, w->temperature,
                            static_cast<unsigned>(w->healthy));
            }
            break;
        }
        case ou::kTypeSysStatus: {
            const auto s = ou::decode<ou::SysStatus>(*frame);
            if (s) {
                std::printf("[系统] batt=%.2fV %.0f%% load=%u.%u%% stream_mask=0x%X\n",
                            s->voltage_total / 1000.0f,
                            static_cast<double>(s->battery_remaining),
                            static_cast<unsigned>(s->load / 10),
                            static_cast<unsigned>(s->load % 10),
                            static_cast<unsigned>(s->stream_mask));
            }
            break;
        }
        default:
            std::printf("[其他] type=0x%02X %zu 字节，跳过\n",
                        static_cast<unsigned>(ou::frame_type(*frame)), frame->size());
            break;
        }
    }
}
