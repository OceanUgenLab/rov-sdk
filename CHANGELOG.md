# 变更日志

本文件遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/) 格式，版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

## [0.2.0] - 2026-08-28

### 破坏性变更

- **协议硬切**：帧格式新增 `ver` 版本字节，v0.2.0 固定为 `0x02`（帧头从 `AA 55 | len | ...` 变为 `AA 55 | ver | len | type | payload | crc16`）。
- **删除字段**：控制指令载荷删除 `servo`（舵机角度）、`arm`（机械臂）、`light[2]`（照明灯）三个字段，改为 `reserved_tail[16]` 扩展预留区（云台/机械臂/照明等后置）。旧 v0.1.0 帧不再兼容。
- **载荷调整**：CmdPacket 52 字节（含 `mode`/`armed`/`reserved[2]` 前导字段 + `target_depth`/`target_heading`/`target_north`/`target_east` 目标量），TelemetryPacket 128 字节（扩展至 22 个标量 + 8 路推进器）。

### 新增

- **schema + codegen + golden 工作流**：`schema/protocol.yaml` 为唯一手写真源，`tools/codegen.py` 生成 C++ 头 / C 头 / 协议文档 / ROS2 `.msg`，`tools/golden_gen.py` 生成 golden 字节向量（`generated/golden/`）。
- **三层架构**：
  - `ou::proto` —— CRC-16/MODBUS、`encodeCmd`/`encodeTele`、`decodeCmd`/`decodeTele`、有状态流式 `FrameParser`（滑动窗口切帧）。
  - `ou::channel` —— 纯字节搬运 `FrameChannel` / `UdpChannel` / `SerialChannel`。
  - `ou::link` —— 组合层 `FrameLink` / `UdpFrameLink` / `SerialFrameLink`，提供 `send_frame` / `recv_frame` / `recv_frame_as<Pkt>` 类型安全收帧。
- **开源交付物**：`LICENSE`（Apache-2.0）、`examples/`、`docs/`（Doxygen + 入口）、`CHANGELOG.md`、`CONTRIBUTING.md`、`.github/`（issue/PR 模板 + CI）、`.devcontainer/`。

### 命名

- 命名空间与产物由 `rovi` 统一更名为 `ou`（库 `ou_proto`/`ou_channel`/`ou_link`，别名 `ou::proto`/`ou::channel`/`ou::link`）。

## [0.1.0] - 2026-08-28

- 初始版本：通信协议编解码（v0.1.0 帧布局，无 `ver` 字段，含 servo/arm/light）。
