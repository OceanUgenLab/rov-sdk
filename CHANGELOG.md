# Changelog

This file follows the [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/) format, and version numbers follow [Semantic Versioning](https://semver.org/lang/zh-CN/).

## [Unreleased]

## [0.2.0] - 2026-08-28

### Breaking changes

- **Hard protocol cut**: the frame format adds a `ver` version byte, fixed to `0x02` in v0.2.0 (the frame header changes from `AA 55 | len | ...` to `AA 55 | ver | len | type | payload | crc16`).
- **Removed fields**: the control command payload drops three fields, `servo` (servo angle), `arm` (robotic arm), and `light[2]` (lights), replacing them with a `reserved_tail[16]` extension area (gimbal / arm / lighting reserved for later). Old v0.1.0 frames are no longer compatible.
- **Payload changes**: CmdPacket is 52 bytes (leading fields `mode`/`armed`/`reserved[2]` plus the targets `target_depth`/`target_heading`/`target_north`/`target_east`), and TelemetryPacket is 128 bytes (expanded to 22 scalars + 8 thrusters).

### Added

- **schema + codegen + golden workflow**: `schema/protocol.yaml` is the single hand-written source of truth; `tools/codegen.py` generates the C++ header / C header / protocol docs / ROS2 `.msg`; `tools/golden_gen.py` generates the golden byte vectors (`generated/golden/`).
- **Three-layer architecture**:
  - `ou::proto`: CRC-16/MODBUS, `encodeCmd`/`encodeTele`, `decodeCmd`/`decodeTele`, and a stateful streaming `FrameParser` (sliding-window frame splitting).
  - `ou::channel`: raw byte transport via `FrameChannel` / `UdpChannel` / `SerialChannel`.
  - `ou::link`: composition layer with `FrameLink` / `UdpFrameLink` / `SerialFrameLink`, providing `send_frame` / `recv_frame` / `recv_frame_as<Pkt>` type-safe receive.
- **Open-source deliverables**: `LICENSE` (Apache-2.0), `examples/`, `docs/` (Doxygen + entry point), `CHANGELOG.md`, `CONTRIBUTING.md`, `.github/` (issue/PR templates + CI), `.devcontainer/`.

### Naming

- The namespace and artifacts were uniformly renamed from `rovi` to `ou` (libraries `ou_proto`/`ou_channel`/`ou_link`, aliases `ou::proto`/`ou::channel`/`ou::link`).

## [0.1.0] - 2026-08-28

- Initial version: communication protocol codec (v0.1.0 frame layout, no `ver` field, includes servo/arm/light).
