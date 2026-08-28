#!/usr/bin/env python3
"""ou_sdk schema-driven code generator.

Reads schema/protocol.yaml and emits:
    generated/ou/protocol.hpp   (C++20 header)
    generated/ou_protocol.h     (C11 packed header)
    generated/docs/protocol.md    (Chinese protocol documentation)
    generated/msg/Cmd.msg         (ROS2 message definition)
    generated/msg/Telemetry.msg   (ROS2 message definition)

All generated files carry the header:
    AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import yaml
from jinja2 import Environment

ROOT = Path(__file__).resolve().parent.parent
SCHEMA_PATH = ROOT / "schema" / "protocol.yaml"
OUT_DIR = ROOT / "generated"

AUTO_HEADER = "AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml"

CPP_TYPE = {"u8": "uint8_t", "f32": "float"}
C_TYPE = {"u8": "uint8_t", "f32": "float"}
MSG_TYPE = {"u8": "uint8", "f32": "float32"}
TYPE_SIZE = {"u8": 1, "f32": 4}


class SchemaError(Exception):
    pass


def load_schema(path: Path) -> dict:
    if not path.exists():
        raise SchemaError(f"schema not found: {path}")
    with path.open("r", encoding="utf-8") as f:
        data = yaml.safe_load(f)
    if not isinstance(data, dict):
        raise SchemaError("schema root must be a mapping")
    _validate_schema(data)
    return data


def _validate_schema(data: dict) -> None:
    required_top = ["version", "frame", "types", "enums", "frames"]
    for key in required_top:
        if key not in data:
            raise SchemaError(f"schema missing top-level key: {key}")

    frames = data["frames"]
    if not isinstance(frames, dict):
        raise SchemaError("schema.frames must be a mapping")
    for frame_name, frame in frames.items():
        if "fields" not in frame:
            raise SchemaError(f"frame {frame_name} missing 'fields'")
        fields = frame["fields"]
        if not isinstance(fields, list):
            raise SchemaError(f"frame {frame_name}.fields must be a list")
        seen_names = set()
        for idx, field in enumerate(fields):
            if not isinstance(field, dict):
                raise SchemaError(f"frame {frame_name} field #{idx} is not a mapping")
            for req in ("name", "type", "unit", "range", "notes"):
                if req not in field:
                    raise SchemaError(
                        f"frame {frame_name} field #{idx} missing required key '{req}'"
                    )
            if field["type"] not in TYPE_SIZE:
                raise SchemaError(
                    f"frame {frame_name} field {field['name']} has unknown type {field['type']!r}"
                )
            if field["name"] in seen_names:
                raise SchemaError(
                    f"frame {frame_name} duplicate field name: {field['name']}"
                )
            seen_names.add(field["name"])


def _camel(name: str) -> str:
    """reserved_tail -> ReservedTail."""
    return "".join(part.capitalize() for part in name.split("_"))


def compute_layout(fields: list[dict]) -> list[dict]:
    """Augment fields with count, element_size, size, offset."""
    offset = 0
    out = []
    for f in fields:
        t = f["type"]
        count = int(f.get("count", 1))
        elem = TYPE_SIZE[t]
        size = elem * count
        out.append({
            "name": f["name"],
            "type": t,
            "count": count,
            "element_size": elem,
            "size": size,
            "offset": offset,
            "unit": f.get("unit", ""),
            "range": f.get("range"),
            "notes": f.get("notes", ""),
        })
        offset += size
    return out


def render_cpp(schema: dict) -> str:
    env = Environment(trim_blocks=True, lstrip_blocks=True)
    template = env.from_string(CPP_TEMPLATE)
    return template.render(ctx=make_ctx(schema))


def render_c(schema: dict) -> str:
    env = Environment(trim_blocks=True, lstrip_blocks=True)
    template = env.from_string(C_TEMPLATE)
    return template.render(ctx=make_ctx(schema))


def render_md(schema: dict) -> str:
    env = Environment(trim_blocks=True, lstrip_blocks=True)
    template = env.from_string(MD_TEMPLATE)
    return template.render(ctx=make_ctx(schema))


def render_msg(schema: dict, frame_name: str) -> str:
    env = Environment(trim_blocks=True, lstrip_blocks=True)
    template = env.from_string(MSG_TEMPLATE)
    return template.render(
        ctx=make_ctx(schema),
        fields=schema["frames"][frame_name]["fields"],
    )


def make_ctx(schema: dict) -> dict:
    frame_meta = schema["frame"]
    cmd = schema["frames"]["CmdPacket"]
    tele = schema["frames"]["TelemetryPacket"]
    cmd_fields = compute_layout(cmd["fields"])
    tele_fields = compute_layout(tele["fields"])
    cmd_size = sum(f["size"] for f in cmd_fields)
    tele_size = sum(f["size"] for f in tele_fields)

    stx = int(str(frame_meta["stx"]), 0)
    stx0 = (stx >> 8) & 0xFF
    stx1 = stx & 0xFF

    # Number of thrusters from the schema thr field count, fallback 8.
    num_thrusters = 8
    for f in tele_fields:
        if f["name"] == "thr":
            num_thrusters = f["count"]
            break

    mode_values = schema["enums"]["Mode"]["values"]

    return {
        "auto_header": AUTO_HEADER,
        "version": schema["version"],
        "frame": frame_meta,
        "stx0": stx0,
        "stx1": stx1,
        "type_cmd": cmd["type"],
        "type_tele": tele["type"],
        "num_thrusters": num_thrusters,
        "frame_overhead": 7,
        "cmd_size": cmd_size,
        "tele_size": tele_size,
        "cmd_fields": cmd_fields,
        "tele_fields": tele_fields,
        "mode_values": mode_values,
        "cpp_type": CPP_TYPE,
        "c_type": C_TYPE,
        "msg_type": MSG_TYPE,
        "camel": _camel,
    }


CPP_TEMPLATE = r"""// {{ ctx.auto_header }}

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ou {

// 帧常量（v0.2.0 固定格式：AA 55 | ver | len | type | payload | crc16）
inline constexpr uint8_t kStx0 = {{ "0x%02X" % ctx.stx0 }};                 // 帧头第一字节
inline constexpr uint8_t kStx1 = {{ "0x%02X" % ctx.stx1 }};                 // 帧头第二字节
inline constexpr uint8_t kProtocolVersion = {{ "0x%02X" % ctx.version }};  // 协议版本
inline constexpr uint8_t kTypeCmd = {{ "0x%02X" % ctx.type_cmd }};          // 控制指令（主控 -> 从控）
inline constexpr uint8_t kTypeTele = {{ "0x%02X" % ctx.type_tele }};       // 遥测（从控 -> 主控）
inline constexpr size_t kNumThrusters = {{ ctx.num_thrusters }};          // 推进器数量
inline constexpr size_t kFrameOverhead = {{ ctx.frame_overhead }};          // 帧固定开销 = STX(2)+ver(1)+len(1)+type(1)+crc(2)
inline constexpr size_t kCmdPayloadSize = {{ ctx.cmd_size }};               // 控制指令载荷字节数
inline constexpr size_t kTelePayloadSize = {{ ctx.tele_size }};             // 遥测载荷字节数

// 控制指令载荷（主控 -> 从控）
struct CmdPacket {
{% for f in ctx.cmd_fields %}
{% if f.count > 1 %}
    {{ ctx.cpp_type[f.type] }} {{ f.name }}[{{ f.count }}]{}; // {{ f.notes }}
{% else %}
    {{ ctx.cpp_type[f.type] }} {{ f.name }}{}; // {{ f.notes }}
{% endif %}
{% endfor %}

    friend bool operator==(const CmdPacket&, const CmdPacket&) = default;
};

// 遥测载荷（从控 -> 主控）
struct TelemetryPacket {
{% for f in ctx.tele_fields %}
{% if f.count > 1 %}
    {{ ctx.cpp_type[f.type] }} {{ f.name }}[{{ f.count }}]{}; // {{ f.notes }}
{% else %}
    {{ ctx.cpp_type[f.type] }} {{ f.name }}{}; // {{ f.notes }}
{% endif %}
{% endfor %}

    friend bool operator==(const TelemetryPacket&, const TelemetryPacket&) = default;
};

// 字段偏移常量（从字段顺序+类型尺寸推导，单位字节）
{% for f in ctx.cmd_fields %}
inline constexpr size_t kCmdPacket{{ ctx.camel(f.name) }}Offset = {{ f.offset }}; // {{ f.name }}
{% endfor %}
{% for f in ctx.tele_fields %}
inline constexpr size_t kTelemetryPacket{{ ctx.camel(f.name) }}Offset = {{ f.offset }}; // {{ f.name }}
{% endfor %}

// 组帧/解码函数声明（实现在 T4 手写）
std::vector<uint8_t> encodeCmd(const CmdPacket& cmd);
std::vector<uint8_t> encodeTele(const TelemetryPacket& tele);
std::optional<CmdPacket> decodeCmd(std::span<const uint8_t> frame);
std::optional<TelemetryPacket> decodeTele(std::span<const uint8_t> frame);

} // namespace ou
"""

C_TEMPLATE = r"""/* {{ ctx.auto_header }} */
#ifndef OU_GENERATED_PROTOCOL_H
#define OU_GENERATED_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 帧常量 */
#define OU_STX0 {{ "0x%02X" % ctx.stx0 }}
#define OU_STX1 {{ "0x%02X" % ctx.stx1 }}
#define OU_PROTOCOL_VERSION {{ "0x%02X" % ctx.version }}
#define OU_TYPE_CMD {{ "0x%02X" % ctx.type_cmd }}
#define OU_TYPE_TELE {{ "0x%02X" % ctx.type_tele }}
#define OU_NUM_THRUSTERS {{ ctx.num_thrusters }}
#define OU_FRAME_OVERHEAD {{ ctx.frame_overhead }}
#define OU_CMD_PAYLOAD_SIZE {{ ctx.cmd_size }}
#define OU_TELE_PAYLOAD_SIZE {{ ctx.tele_size }}

/* 字段偏移宏 */
{% for f in ctx.cmd_fields %}
#define OU_CMDPACKET_{{ f.name.upper() }}_OFFSET {{ f.offset }}
{% endfor %}
{% for f in ctx.tele_fields %}
#define OU_TELEMETRYPACKET_{{ f.name.upper() }}_OFFSET {{ f.offset }}
{% endfor %}

/* 控制指令载荷（主控 -> 从控） */
typedef struct __attribute__((packed)) {
{% for f in ctx.cmd_fields %}
{% if f.count > 1 %}
    {{ ctx.c_type[f.type] }} {{ f.name }}[{{ f.count }}]; /* {{ f.notes }} */
{% else %}
    {{ ctx.c_type[f.type] }} {{ f.name }}; /* {{ f.notes }} */
{% endif %}
{% endfor %}
} CmdPacket;

/* 遥测载荷（从控 -> 主控） */
typedef struct __attribute__((packed)) {
{% for f in ctx.tele_fields %}
{% if f.count > 1 %}
    {{ ctx.c_type[f.type] }} {{ f.name }}[{{ f.count }}]; /* {{ f.notes }} */
{% else %}
    {{ ctx.c_type[f.type] }} {{ f.name }}; /* {{ f.notes }} */
{% endif %}
{% endfor %}
} TelemetryPacket;

_Static_assert(sizeof(CmdPacket) == {{ ctx.cmd_size }}, "CmdPacket size mismatch");
_Static_assert(sizeof(TelemetryPacket) == {{ ctx.tele_size }}, "TelemetryPacket size mismatch");

#ifdef __cplusplus
}
#endif

#endif /* OU_GENERATED_PROTOCOL_H */
"""

MD_TEMPLATE = r"""<!-- {{ ctx.auto_header }} -->

# OU 通信协议（v0.2.0）

> 本文件由 `tools/codegen.py` 从 `schema/protocol.yaml` 自动生成。三端（上位机 SDK、算力板、STM32 固件）以 `schema/protocol.yaml` 为唯一权威源。

## 1. 帧格式

```
AA 55 | ver(1B) | len(1B) | type(1B) | payload(len) | crc16(2B, 小端)
```

| 字段 | 长度 | 说明 |
|------|------|------|
| STX0 | 1B | 固定 `0xAA` |
| STX1 | 1B | 固定 `0x55` |
| ver | 1B | 协议版本，v0.2.0 固定 `0x02` |
| len | 1B | payload 字节数（不含帧头、不含 CRC） |
| type | 1B | 帧类型：`0x01` 控制指令 / `0x02` 遥测 |
| payload | len | 载荷（见下） |
| crc16 | 2B | CRC-16/MODBUS，小端；覆盖范围从 `ver` 起，长度 `3 + len` 字节（不含 STX） |

- 无 ETX 结束符。
- 帧总长 = `7 + len`。
- **字节序固定小端**（与 STM32 一致），浮点按 IEEE-754 float32 位模式传输。

## 2. CRC-16/MODBUS

- 多项式（反射形式）`0xA001`，初始值 `0xFFFF`，反射输入/输出，无 xorout。
- 标准校验向量：`crc16("123456789") == 0x{{ "%04X" % ctx.frame.crc.known_answer }}`。
- 覆盖范围：`ver`、`len`、`type`、`payload`，即从 `ver` 起共 `3 + len` 字节。

## 3. 模式枚举

| 常量 | 值 | 说明 |
|------|----|------|
{% for name, val in ctx.mode_values.items() %}
| `MODE_{{ name }}` | {{ val }} | {{ {0: '手动遥控', 1: '自动控制', 2: '一键返航', 3: '悬停保持'}.get(val, '') }} |
{% endfor %}

## 4. 载荷定义

### 4.1 控制指令 `type=0x01`（主控 -> 从控），payload {{ ctx.cmd_size }} 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
{% for f in ctx.cmd_fields %}
| {{ f.offset }} | {{ f.name }}{% if f.count > 1 %}[0..{{ f.count-1 }}]{% endif %} | {{ ctx.msg_type[f.type] }}{% if f.count > 1 %}×{{ f.count }}{% endif %} | {{ f.unit or '""' }} | {{ f.range }} | {{ f.notes }} |
{% endfor %}

### 4.2 遥测 `type=0x02`（从控 -> 主控），payload {{ ctx.tele_size }} 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
{% for f in ctx.tele_fields %}
| {{ f.offset }} | {{ f.name }}{% if f.count > 1 %}[0..{{ f.count-1 }}]{% endif %} | {{ ctx.msg_type[f.type] }}{% if f.count > 1 %}×{{ f.count }}{% endif %} | {{ f.unit or '""' }} | {{ f.range }} | {{ f.notes }} |
{% endfor %}

## 5. 实现约束

- 解析必须**滑动窗口**处理流（容忍噪声前缀/粘包），STX 不匹配时逐字节推进。
- `ver`/`len`/`type`/CRC 任一校验失败即丢弃该帧（跳到下一帧边界）。
- 载荷校验失败时**不修改**输出缓冲区。
"""

MSG_TEMPLATE = r"""# {{ ctx.auto_header }}

# Mode 枚举
{% for name, val in ctx.mode_values.items() %}
uint8 MODE_{{ name }}={{ val }}
{% endfor %}

# Payload fields
{% for f in fields %}
{% set count = f.get('count', 1) %}
{% if count > 1 %}
{{ ctx.msg_type[f.type] }}[{{ count }}] {{ f.name }}
{% else %}
{{ ctx.msg_type[f.type] }} {{ f.name }}
{% endif %}
{% endfor %}
"""


def write_if_changed(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return
    path.write_text(content, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate ou protocol artifacts from schema.")
    parser.add_argument(
        "--schema",
        type=Path,
        default=SCHEMA_PATH,
        help="path to protocol.yaml",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=OUT_DIR,
        help="output directory",
    )
    args = parser.parse_args()

    schema = load_schema(args.schema)
    out = args.out_dir

    write_if_changed(out / "ou" / "protocol.hpp", render_cpp(schema))
    write_if_changed(out / "ou_protocol.h", render_c(schema))
    write_if_changed(out / "docs" / "protocol.md", render_md(schema))
    write_if_changed(out / "msg" / "Cmd.msg", render_msg(schema, "CmdPacket"))
    write_if_changed(out / "msg" / "Telemetry.msg", render_msg(schema, "TelemetryPacket"))

    print(f"Generated artifacts in {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
