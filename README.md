# ou_sdk — 水下机器人上位机侧 SDK

C++20 实现的水下机器人上位机侧 SDK，提供三端（上位机 / 算力板 / STM32 固件）共享的**通信协议编解码**与**可插拔传输层**。

[![CI](https://github.com/OceanUgenLab/rov-sdk/actions/workflows/ci.yml/badge.svg)](https://github.com/OceanUgenLab/rov-sdk/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/License-Apache--2.0-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-%E2%89%A53.16-064F8C?style=flat-square&logo=cmake)](https://cmake.org/)

## 快速上手

依赖：CMake ≥ 3.16、C++20 编译器、Ninja（推荐）。无任何外部库依赖，纯标准库 + POSIX 原生 API。

```bash
# 克隆
git clone https://github.com/OceanUgenLab/rov-sdk.git
cd rov-sdk

# 配置 + 构建
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 测试（3 个独立测试全部通过）
ctest --test-dir build --output-on-failure
```

### 第一段代码：上位机发一条控制指令

```cpp
#include <ou/protocol.hpp>    // CmdPacket、encodeCmd
#include <ou/udp_channel.hpp> // UdpChannel
#include <ou/frame_link.hpp>  // UdpFrameLink

int main() {
    ou::UdpChannel channel;
    channel.bind(0);                        // 本地端口自动分配
    channel.set_peer("192.168.2.2", 8081);  // 对端：从控地址

    ou::UdpFrameLink link(channel);

    ou::CmdPacket cmd;
    cmd.armed = 1;      // 解锁
    cmd.surge  = 0.5f;  // 前进半速

    auto frame = ou::encodeCmd(cmd);   // 编码为完整帧（含 CRC）
    link.send_frame(frame);            // 发送
}
```

更完整的可运行示例见 [`examples/`](examples/)：

- [`examples/host_command_sender/`](examples/host_command_sender/) —— 上位机发指令
- [`examples/board_telemetry_receiver/`](examples/board_telemetry_receiver/) —— 算力板收遥测

## 架构分层

```
┌─────────────────────────────────────────────────────────┐
│  link  组合层  (ou::link)                                │
│  FrameLink / UdpFrameLink / SerialFrameLink              │
│  send_frame · recv_frame · recv_frame_as<Pkt>（类型安全） │
├─────────────────────────────────────────────────────────┤
│  channel  字节搬运层  (ou::channel)                       │
│  FrameChannel / UdpChannel / SerialChannel               │
│  纯字节收发，不组帧、不解析 STX/CRC                       │
├─────────────────────────────────────────────────────────┤
│  proto  编解码层  (ou::proto)                            │
│  crc16 · encodeCmd/Tele · decodeCmd/Tele · FrameParser   │
│  有状态流式切帧（滑动窗口，容忍噪声/粘包）                  │
└─────────────────────────────────────────────────────────┘
```

| 层 | 库 | 别名 | 职责 |
|----|----|------|------|
| proto | `ou_proto` | `ou::proto` | CRC-16/MODBUS、帧编解码、有状态 `FrameParser` 流式切帧 |
| channel | `ou_channel` | `ou::channel` | 纯字节搬运通道（UDP / 串口），不组帧、不解析 |
| link | `ou_link` | `ou::link` | 组合层：持有 channel + FrameParser，提供 `send_frame` / `recv_frame` / `recv_frame_as<Pkt>` 类型安全收帧 |

`ou::link` 通过 PUBLIC 链接传递 `ou::proto` + `ou::channel`，消费方链接 `ou::link` 一处即可。

## 协议

帧格式 `AA 55 | ver | len | type | payload | crc16`（v0.2.0，`ver=0x02`）的完整定义与 golden 帧向量见 [`generated/docs/protocol.md`](generated/docs/protocol.md)。协议为三端（上位机 SDK / 算力板 / STM32 固件）公共接口，本仓库为实现与文档持有方。

**协议修改的唯一路径**（详见 [`CONTRIBUTING.md`](CONTRIBUTING.md)）：

```
改 schema/protocol.yaml  →  tools/codegen.py  →  tools/golden_gen.py  →  三端派生同步
```

`schema/protocol.yaml` 是唯一手写真源，`generated/` 下所有产物（C++ 头 / C 头 / 协议文档 / ROS2 `.msg` / golden 向量）均为派生结果，禁止手改。

## 集成

```cmake
add_subdirectory(rov-sdk)   # 或 find_package(ou_sdk)
target_link_libraries(app PRIVATE ou::link)
```

`examples/` 演示了不依赖 install、直接 `add_subdirectory` 引 SDK 根目录的最小集成方式。

## 项目结构

```
rov-sdk/
├── include/ou/            # 手写公共头（proto/channel/link）
├── src/                   # 三库实现
├── schema/protocol.yaml   # 协议唯一手写真源
├── generated/             # codegen 派生产物（勿手改）
│   ├── ou/protocol.hpp    # C++ 头
│   ├── ou_protocol.h      # C 头
│   ├── docs/protocol.md   # 协议文档
│   ├── msg/               # ROS2 .msg
│   └── golden/            # golden 字节向量
├── tools/                 # codegen.py / golden_gen.py
├── examples/              # 两个最小可编译示例
├── docs/                  # Doxygen 配置 + 文档入口
├── tests/                 # 独立测试可执行程序（ctest）
├── .github/               # issue/PR 模板 + CI 工作流
└── .devcontainer/         # VS Code 开发容器
```

## 文档

- [`docs/index.md`](docs/index.md) —— 文档入口
- [`generated/docs/protocol.md`](generated/docs/protocol.md) —— 协议权威文档（自动生成）
- [`CHANGELOG.md`](CHANGELOG.md) / [`CONTRIBUTING.md`](CONTRIBUTING.md)

生成 API 参考：`doxygen docs/Doxyfile`，产物输出到 `build/doxygen/html/`。

## 许可

[Apache License 2.0](LICENSE) © 2026 OceanUgenLab
