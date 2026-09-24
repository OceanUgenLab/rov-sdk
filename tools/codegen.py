#!/usr/bin/env python3
"""ou_sdk schema-driven code generator.

Reads schema/protocol.yaml and emits:
    generated/ou/protocol.hpp   (C++20 header, self-contained encode/decode)
    generated/ou_protocol.h     (C11 packed header)
    generated/docs/protocol.md  (Chinese protocol documentation)
    generated/msg/<Frame>.msg   (ROS2 message definition, one per frame)

All generated files carry the header:
    AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import yaml
from jinja2 import Environment

ROOT = Path(__file__).resolve().parent.parent
SCHEMA_PATH = ROOT / "schema" / "protocol.yaml"
OUT_DIR = ROOT / "generated"

AUTO_HEADER = "AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml"

CPP_TYPE = {
    "u8": "uint8_t", "u16": "uint16_t", "u32": "uint32_t", "u64": "uint64_t",
    "i8": "int8_t", "i16": "int16_t", "i32": "int32_t",
    "f32": "float", "char": "char",
}
C_TYPE = CPP_TYPE
MSG_TYPE = {
    "u8": "uint8", "u16": "uint16", "u32": "uint32", "u64": "uint64",
    "i8": "int8", "i16": "int16", "i32": "int32",
    "f32": "float32", "char": "char",
}
TYPE_SIZE = {
    "u8": 1, "u16": 2, "u32": 4, "u64": 8,
    "i8": 1, "i16": 2, "i32": 4,
    "f32": 4, "char": 1,
}


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
    if not isinstance(frames, dict) or not frames:
        raise SchemaError("schema.frames must be a non-empty mapping")

    seen_types = {}
    for frame_name, frame in frames.items():
        if "type" not in frame or "fields" not in frame:
            raise SchemaError(f"frame {frame_name} missing 'type' or 'fields'")
        ftype = int(str(frame["type"]), 0)
        if ftype in seen_types:
            raise SchemaError(
                f"frame {frame_name} reuses type 0x{ftype:02X} of {seen_types[ftype]}"
            )
        seen_types[ftype] = frame_name
        if not isinstance(frame["fields"], list):
            raise SchemaError(f"frame {frame_name}.fields must be a list")
        seen_names = set()
        for idx, field in enumerate(frame["fields"]):
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
        payload = sum(TYPE_SIZE[f["type"]] * int(f.get("count", 1)) for f in frame["fields"])
        if payload > 255:
            raise SchemaError(f"frame {frame_name} payload {payload}B exceeds 255B")


def _camel(name: str) -> str:
    """snake_case -> SnakeCase；已是大驼峰（SystemState）原样保留。"""
    name = str(name)
    if "_" not in name:
        return name
    return _camel_cc(name)


def _camel_cc(name: str) -> str:
    """任意命名 -> 逐词大驼峰：MANUAL -> Manual，CMD_SET_MODE -> CmdSetMode。"""
    return "".join(p[:1].upper() + p[1:].lower() for p in str(name).split("_"))


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


def make_ctx(schema: dict) -> dict:
    frame_meta = schema["frame"]
    stx = int(str(frame_meta["stx"]), 0)
    frames = []
    for name, frame in schema["frames"].items():
        layout = compute_layout(frame["fields"])
        frames.append({
            "name": name,
            "type": int(str(frame["type"]), 0),
            "direction": frame.get("direction", ""),
            "notes": frame.get("notes", ""),
            "fields": layout,
            "payload_size": sum(f["size"] for f in layout),
        })
    enums = [
        {"name": name, "type": e["type"], "pairs": list(e["values"].items())}
        for name, e in schema.get("enums", {}).items()
    ]
    return {
        "auto_header": AUTO_HEADER,
        "version": int(str(schema["version"]), 0),
        "frame": frame_meta,
        "stx0": (stx >> 8) & 0xFF,
        "stx1": stx & 0xFF,
        "frames": frames,
        "enums": enums,
        "cpp_type": CPP_TYPE,
        "c_type": C_TYPE,
        "msg_type": MSG_TYPE,
        "camel": _camel, "camel_cc": _camel_cc,
    }


def _render(template_src: str, ctx: dict, **extra):
    env = Environment(trim_blocks=True, lstrip_blocks=True)
    return env.from_string(template_src).render(ctx=ctx, **extra)


def render_cpp(schema: dict) -> str:
    return _render(CPP_TEMPLATE, make_ctx(schema))


def render_c(schema: dict) -> str:
    return _render(C_TEMPLATE, make_ctx(schema))


def render_md(schema: dict) -> str:
    return _render(MD_TEMPLATE, make_ctx(schema))


def render_msg(schema: dict, frame_name: str) -> str:
    ctx = make_ctx(schema)
    frame = next(f for f in ctx["frames"] if f["name"] == frame_name)
    return _render(MSG_TEMPLATE, ctx, frame=frame)


CPP_TEMPLATE = r"""// {{ ctx.auto_header }}

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

namespace ou {

namespace detail {

// 字段访问器：标量直接返回，数组取首元素指针（用于生成器统一展开）
template <typename T, size_t N>
inline constexpr const T* field_ptr(const T (&v)[N]) { return v; }
template <typename T>
inline constexpr const T* field_ptr(const T& v) { return &v; }
template <typename T, size_t N>
inline constexpr T* field_ptr(T (&v)[N]) { return v; }
template <typename T>
inline constexpr T* field_ptr(T& v) { return &v; }
template <typename T, size_t N>
inline constexpr size_t field_count(const T (&)[N]) { return N; }
template <typename T>
inline constexpr size_t field_count(const T&) { return 1; }

// 小端字节读写（无 strict-aliasing UB）
inline void putBytes(std::vector<uint8_t>& out, const uint8_t* p, size_t n) {
    out.insert(out.end(), p, p + n);
}
inline uint64_t getUintLE(std::span<const uint8_t> s, size_t off, size_t n) {
    uint64_t v = 0;
    for (size_t i = 0; i < n; ++i) v |= static_cast<uint64_t>(s[off + i]) << (8 * i);
    return v;
}

}  // namespace detail

// CRC-16/MODBUS（init 0xFFFF, poly 0xA001 反射），实现在 src/protocol.cpp
uint16_t crc16(std::span<const uint8_t> data);

// ---------------------------------------------------------------------------
// 帧常量（v{{ "0x%02X" % ctx.version }} 帧格式：AA 55 | ver | len | type | payload | crc16）
// ---------------------------------------------------------------------------
inline constexpr uint8_t kStx0 = {{ "0x%02X" % ctx.stx0 }};
inline constexpr uint8_t kStx1 = {{ "0x%02X" % ctx.stx1 }};
inline constexpr uint8_t kProtocolVersion = {{ "0x%02X" % ctx.version }};
inline constexpr size_t kFrameOverhead = 7;  // STX(2)+ver(1)+len(1)+type(1)+crc(2)

{% for fr in ctx.frames %}
// {{ fr.notes }}
inline constexpr uint8_t kType{{ fr.name }} = {{ "0x%02X" % fr.type }};
inline constexpr size_t k{{ fr.name }}PayloadSize = {{ fr.payload_size }};
{% endfor %}

// ---------------------------------------------------------------------------
// 枚举常量
// ---------------------------------------------------------------------------
{% for e in ctx.enums %}
enum class {{ ctx.camel(e.name) }} : {{ ctx.cpp_type[e.type] }} {
{% for name, val in e.pairs %}
    k{{ ctx.camel_cc(name) }} = {{ val }},
{% endfor %}
};
{% endfor %}

// ---------------------------------------------------------------------------
// 载荷结构体
// ---------------------------------------------------------------------------
{% for fr in ctx.frames %}
// {{ fr.notes }}，payload {{ fr.payload_size }} 字节
struct {{ fr.name }} {
{% for f in fr.fields %}
{% if f.count > 1 %}
    {{ ctx.cpp_type[f.type] }} {{ f.name }}[{{ f.count }}]{}; // {{ f.notes }}
{% else %}
    {{ ctx.cpp_type[f.type] }} {{ f.name }}{}; // {{ f.notes }}
{% endif %}
{% endfor %}

    friend bool operator==(const {{ fr.name }}&, const {{ fr.name }}&) = default;
};

{% endfor %}

// ---------------------------------------------------------------------------
// 字段偏移常量（从字段顺序+类型尺寸推导，单位字节）
// ---------------------------------------------------------------------------
{% for fr in ctx.frames %}
{% for f in fr.fields %}
inline constexpr size_t k{{ fr.name }}{{ ctx.camel(f.name) }}Offset = {{ f.offset }}; // {{ fr.name }}.{{ f.name }}
{% endfor %}
{% endfor %}

// ---------------------------------------------------------------------------
// 生成式编解码：逐字段小端展开（自包含，无需手写 cpp）
// ---------------------------------------------------------------------------
namespace detail {

inline std::vector<uint8_t> makeFrame(std::span<const uint8_t> payload, uint8_t type) {
    std::vector<uint8_t> out;
    out.reserve(kFrameOverhead + payload.size());
    out.push_back(kStx0);
    out.push_back(kStx1);
    out.push_back(kProtocolVersion);
    out.push_back(static_cast<uint8_t>(payload.size()));
    out.push_back(type);
    out.insert(out.end(), payload.begin(), payload.end());
    const uint16_t crc = crc16({out.data() + 2, 3 + payload.size()});
    out.push_back(static_cast<uint8_t>(crc & 0xFF));
    out.push_back(static_cast<uint8_t>(crc >> 8));
    return out;
}

inline bool validateFrame(std::span<const uint8_t> frame, uint8_t type, size_t payload_size) {
    const size_t total = kFrameOverhead + payload_size;
    if (frame.size() != total) return false;
    if (frame[0] != kStx0 || frame[1] != kStx1) return false;
    if (frame[2] != kProtocolVersion) return false;
    if (frame[3] != payload_size) return false;
    if (frame[4] != type) return false;
    const uint16_t crc = crc16({frame.data() + 2, 3 + payload_size});
    const uint16_t got = static_cast<uint16_t>(frame[total - 2]) |
                         static_cast<uint16_t>(static_cast<uint16_t>(frame[total - 1]) << 8);
    return crc == got;
}

}  // namespace detail

{% for fr in ctx.frames %}
// {{ fr.name }}：编码（组完整帧）
inline std::vector<uint8_t> encode{{ fr.name }}(const {{ fr.name }}& p) {
    std::vector<uint8_t> payload;
    payload.reserve(k{{ fr.name }}PayloadSize);
{% for f in fr.fields %}
    { // {{ f.name }}
        [[maybe_unused]] constexpr size_t N = {{ f.count }}{% if f.element_size != 1 %}, W = {{ f.element_size }}{% endif %};
        auto* src = detail::field_ptr(p.{{ f.name }});
{% if f.type == 'f32' %}
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
{% elif f.element_size == 1 %}
        for (size_t i = 0; i < {{ f.count }}; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
{% else %}
        static_assert(sizeof({{ ctx.cpp_type[f.type] }}) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<{{ ctx.cpp_type[f.type] }}>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
{% endif %}
    }
{% endfor %}
    return detail::makeFrame(payload, kType{{ fr.name }});
}

// {{ fr.name }}：解码（完整帧 -> 载荷）
inline std::optional<{{ fr.name }}> decode{{ fr.name }}(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kType{{ fr.name }}, k{{ fr.name }}PayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    {{ fr.name }} out{};
{% for f in fr.fields %}
    { // {{ f.name }}
        constexpr size_t N = {{ f.count }}, W = {{ f.element_size }};
        auto* dst = detail::field_ptr(out.{{ f.name }});
{% if f.type == 'f32' %}
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + {{ f.offset }} + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
{% else %}
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + {{ f.offset }} + i * W, W);
            dst[i] = static_cast<{{ ctx.cpp_type[f.type] }}>(raw);
        }
{% endif %}
    }
{% endfor %}
    return out;
}

{% endfor %}

// ---------------------------------------------------------------------------
// 类型 -> type 字节 / 编解码函数映射（供 FrameLink::recv_frame_as 等泛型接口）
// ---------------------------------------------------------------------------
template <typename Pkt> struct packet_traits;
{% for fr in ctx.frames %}
template <>
struct packet_traits<{{ fr.name }}> {
    static constexpr uint8_t type = kType{{ fr.name }};
    static constexpr size_t payload_size = k{{ fr.name }}PayloadSize;
    static constexpr auto encode = encode{{ fr.name }};
    static constexpr auto decode = decode{{ fr.name }};
};
{% endfor %}

}  // namespace ou
"""

C_TEMPLATE = r"""/* {{ ctx.auto_header }} */
#ifndef OU_GENERATED_PROTOCOL_H
#define OU_GENERATED_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 帧常量（v{{ "0x%02X" % ctx.version }}：AA 55 | ver | len | type | payload | crc16） */
#define OU_STX0 {{ "0x%02X" % ctx.stx0 }}
#define OU_STX1 {{ "0x%02X" % ctx.stx1 }}
#define OU_PROTOCOL_VERSION {{ "0x%02X" % ctx.version }}
#define OU_FRAME_OVERHEAD 7

{% for fr in ctx.frames %}
/* {{ fr.notes }} */
#define OU_TYPE_{{ fr.name.upper() }} {{ "0x%02X" % fr.type }}
#define OU_{{ fr.name.upper() }}_PAYLOAD_SIZE {{ fr.payload_size }}

{% endfor %}
{% for fr in ctx.frames %}
/* {{ fr.name }} 字段偏移 */
{% for f in fr.fields %}
#define OU_{{ fr.name.upper() }}_{{ f.name.upper() }}_OFFSET {{ f.offset }}
{% endfor %}
{% endfor %}

{% for e in ctx.enums %}
/* {{ ctx.camel(e.name) }} 枚举 */
{% for name, val in e.pairs %}
#define OU_{{ e.name.upper() }}_{{ name }} {{ val }}
{% endfor %}
{% endfor %}

{% for fr in ctx.frames %}
/* {{ fr.notes }}，payload {{ fr.payload_size }} 字节 */
typedef struct __attribute__((packed)) {
{% for f in fr.fields %}
{% if f.count > 1 %}
    {{ ctx.c_type[f.type] }} {{ f.name }}[{{ f.count }}]; /* {{ f.notes }} */
{% else %}
    {{ ctx.c_type[f.type] }} {{ f.name }}; /* {{ f.notes }} */
{% endif %}
{% endfor %}
} {{ fr.name }};

_Static_assert(sizeof({{ fr.name }}) == {{ fr.payload_size }}, "{{ fr.name }} size mismatch");

{% endfor %}

#ifdef __cplusplus
}
#endif

#endif /* OU_GENERATED_PROTOCOL_H */
"""

MD_TEMPLATE = r"""<!-- {{ ctx.auto_header }} -->

# OU 通信协议（v{{ "0x%02X" % ctx.version }}）

> 本文件由 `tools/codegen.py` 从 `schema/protocol.yaml` 自动生成。三端（上位机 SDK、算力板、STM32 固件）以 `schema/protocol.yaml` 为唯一权威源。

## 1. 帧格式

```
AA 55 | ver(1B) | len(1B) | type(1B) | payload(len) | crc16(2B, 小端)
```

| 字段 | 长度 | 说明 |
|------|------|------|
| STX0 | 1B | 固定 `0xAA` |
| STX1 | 1B | 固定 `0x55` |
| ver | 1B | 协议版本，固定 `0x{{ "%02X" % ctx.version }}` |
| len | 1B | payload 字节数（不含帧头、不含 CRC） |
| type | 1B | 帧类型（见帧清单） |
| payload | len | 载荷（见下） |
| crc16 | 2B | CRC-16/MODBUS，小端；覆盖范围从 `ver` 起共 `3 + len` 字节（不含 STX） |

- 无 ETX 结束符。帧总长 = `7 + len`。
- **字节序固定小端**（与 STM32 一致），浮点按 IEEE-754 float32 位模式传输。

## 2. CRC-16/MODBUS

- 多项式（反射形式）`0xA001`，初始值 `0xFFFF`，反射输入/输出，无 xorout。
- 标准校验向量：`crc16("123456789") == 0x{{ "%04X" % ctx.frame.crc.known_answer }}`。

## 3. 枚举

{% for e in ctx.enums %}
### {{ ctx.camel(e.name) }}（{{ e.type }}）

| 常量 | 值 |
|------|----|
{% for name, val in e.pairs %}
| `{{ e.name.upper() }}_{{ name }}` | {{ val }} |
{% endfor %}

{% endfor %}

## 4. 帧清单

| type | 帧 | 方向 | payload | 说明 |
|------|----|------|---------|------|
{% for fr in ctx.frames %}
| `0x{{ "%02X" % fr.type }}` | {{ fr.name }} | {{ fr.direction }} | {{ fr.payload_size }}B | {{ fr.notes }} |
{% endfor %}

## 5. 载荷定义

{% for fr in ctx.frames %}
### {{ fr.name }} `type=0x{{ "%02X" % fr.type }}`，payload {{ fr.payload_size }} 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
{% for f in fr.fields %}
| {{ f.offset }} | {{ f.name }}{% if f.count > 1 %}[0..{{ f.count-1 }}]{% endif %} | {{ f.type }}{% if f.count > 1 %}×{{ f.count }}{% endif %} | {{ f.unit or '""' }} | {{ f.range }} | {{ f.notes }} |
{% endfor %}

{% endfor %}

## 6. 实现约束

- 解析必须**滑动窗口**处理流（容忍噪声前缀/粘包），STX 不匹配时逐字节推进。
- `ver`/`len`/`type`/CRC 任一校验失败即丢弃该帧（跳到下一帧边界）。
- 载荷校验失败时**不修改**输出缓冲区。
"""

MSG_TEMPLATE = r"""# {{ ctx.auto_header }}

# {{ frame.notes }}
uint8 FRAME_TYPE=0x{{ "%02X" % frame.type }}

# Payload fields
{% for f in frame.fields %}
{% if f.count > 1 %}
{{ ctx.msg_type[f.type] }}[{{ f.count }}] {{ f.name }}
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
    parser.add_argument("--schema", type=Path, default=SCHEMA_PATH)
    parser.add_argument("--out-dir", type=Path, default=OUT_DIR)
    args = parser.parse_args()

    schema = load_schema(args.schema)
    out = args.out_dir

    write_if_changed(out / "ou" / "protocol.hpp", render_cpp(schema))
    write_if_changed(out / "ou_protocol.h", render_c(schema))
    write_if_changed(out / "docs" / "protocol.md", render_md(schema))
    for frame_name in schema["frames"]:
        write_if_changed(out / "msg" / f"{frame_name}.msg", render_msg(schema, frame_name))

    print(f"Generated artifacts in {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
