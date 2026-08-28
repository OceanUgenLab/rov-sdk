# ou_sdk 文档

## 协议文档

- [通信协议 v0.2.0（自动生成）](../generated/docs/protocol.md) —— 帧格式、CRC、载荷定义的权威文档，由 `tools/codegen.py` 从 `schema/protocol.yaml` 生成。

> `schema/protocol.yaml` 是三端（上位机 SDK、算力板、STM32 固件）的**唯一手写真源**。`generated/` 下的一切产物都是派生结果，请勿手改。

## API 参考

生成 Doxygen 文档：

```bash
doxygen docs/Doxyfile
```

产物输出到 `build/doxygen/html/index.html`。

覆盖范围（`docs/Doxyfile` 的 `INPUT`）：

| 路径 | 说明 |
|------|------|
| `include/` | 手写公共头：`proto.hpp`（FrameParser / packet_traits / decode）、`channel.hpp`、`udp_channel.hpp`、`serial_channel.hpp`、`frame_link.hpp` |
| `generated/ou/` | codegen 生成头：`protocol.hpp`（CmdPacket / TelemetryPacket / 常量 / encode / decode 声明） |

## 三层架构

| 层 | 库 | 职责 |
|----|----|------|
| proto | `ou::proto` | 编解码 + 有状态 `FrameParser` 流式切帧 |
| channel | `ou::channel` | 纯字节搬运（`UdpChannel` / `SerialChannel`），不组帧、不解析 |
| link | `ou::link` | 组合层：`FrameLink` / `UdpFrameLink` / `SerialFrameLink`，提供 `send_frame` / `recv_frame` / `recv_frame_as<Pkt>` 类型安全收帧 |

## 示例

- [`examples/host_command_sender/`](../examples/host_command_sender/) —— 上位机发控制指令
- [`examples/board_telemetry_receiver/`](../examples/board_telemetry_receiver/) —— 算力板收遥测

## 其他

- [CHANGELOG](../CHANGELOG.md)
- [CONTRIBUTING](../CONTRIBUTING.md)
