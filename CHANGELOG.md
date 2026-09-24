# 变更日志

本文件遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/) 格式，版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 破坏性变更（协议 v0.3.0，P0 帧体系）

- **协议硬切**：`ver` 固定为 `0x03`，旧 v0.2.0 帧作废。删除 `CmdPacket`(0x01) / `TelemetryPacket`(0x02)，两号不回收；三端（SDK / 算力板 / 固件）一次切换，不设过渡期。
- **新增 16 帧**（P0）：
  - 批次 A 命令闭环：`Heartbeat`(0x50)、`Command`(0x32)、`CommandAck`(0x51)、`ManualControl`(0x30)；
  - 批次 B 遥测：`SysStatus`(0x10，含电池与 stream_mask)、`PoseNed`(0x13)、`EkfStatusReport`(0x14)、`VfrHud`(0x15)、`GlobalPositionInt`(0x16)、`GpsRawInt`(0x18，默认关)、`WaterDepth`(0x19)、`DistanceSensor`(0x52)、`RcChannels`(0x1A，默认关)、`ServoOutputRaw`(0x1B，默认关)；
  - 批次 C 参数：`ParamSet`(0x33)、`ParamValue`(0x1D)。
  - 默认关闭的三帧由 `CMD_SET_STREAM` 命令按需开启，开关状态经 `SysStatus.stream_mask` 广播。
- **架构变化**：帧编解码全部由 codegen 在 `generated/ou/protocol.hpp` 内联生成（支持 u8..u64/i8..i32/f32/char 与定长数组）；`src/protocol.cpp` 仅保留 CRC 与 FrameParser；`packet_traits` 挂载 encode/decode 函数引用，泛型 `ou::encode<Pkt>` / `ou::decode<Pkt>`。
- ROS2 `.msg` 生成由固定 2 个改为逐帧生成（`generated/msg/<Frame>.msg`）。

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
