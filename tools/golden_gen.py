#!/usr/bin/env python3
"""ou_sdk schema-driven golden vector generator.

Reads schema/protocol.yaml and emits:
    generated/golden/golden.h   (C-compatible byte arrays)
    generated/golden/golden.json (human/tool-readable cases)

All generated files carry the header:
    AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

Frame layout (v0.3.0): AA 55 | ver(1B) | len(1B) | type(1B) | payload | crc16(2B LE)
CRC-16/MODBUS covers bytes from ver to end of payload (3 + len bytes total).
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

# Import schema helpers from codegen.py (same repo, same Python path edge cases).
_HERE = Path(__file__).resolve().parent
_ROOT = _HERE.parent
sys.path.insert(0, str(_HERE))
from codegen import compute_layout, load_schema

AUTO_HEADER = "AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml"
SCHEMA_PATH = _ROOT / "schema" / "protocol.yaml"
OUT_DIR = _ROOT / "generated" / "golden"

STRUCT_FMT = {
    "u8": "B", "u16": "H", "u32": "I", "u64": "Q",
    "i8": "b", "i16": "h", "i32": "i", "f32": "f", "char": "s",
}

# 固定测试输入：浮点均选 IEEE-754 float32 可精确表示的值；未列出的字段取 0。
FRAME_INPUTS = {
    "Heartbeat": {
        "mode": 1,            # AUTO
        "system_type": 0x00,  # 机器人 / 机型 0
        "fw_version": 3,
        "system_state": 4,    # ARMED
    },
    "SysStatus": {
        "sensors_present": 0x0000010F,
        "sensors_enabled": 0x0000010F,
        "sensors_health": 0x0000010F,
        "load": 125,
        "voltage_total": 29600,
        "voltage_cell_max": 4200,
        "voltage_cell_min": 3900,
        "current_battery": 250,
        "battery_remaining": 75,
        "current_consumed": 5000,
        "battery_temperature": 2500,
        "battery_fault_bitmask": 0,
        "drop_rate_comm": 0,
        "errors_comm": 0,
        "errors_count": [1, 2, 3, 4],
        "stream_mask": 5,  # GPS + SERVO 开
    },
    "CommandAck": {
        "command": 2,  # CMD_ARM
        "result": 0,   # 接受
        "progress": 100,
        "result_param2": 0,
    },
    "PoseNed": {
        "time_boot_ms": 123456,
        "roll": 0.5,
        "pitch": -0.25,
        "yaw": 3.0,
        "rollspeed": 0.125,
        "pitchspeed": -0.125,
        "yawspeed": 0.5,
        "x": 10.0,
        "y": -10.0,
        "z": 12.5,
        "vx": 0.5,
        "vy": -0.5,
        "vz": 0.25,
    },
    "EkfStatusReport": {
        "flags": 0x0003,
        "velocity_variance": 10,
        "pos_horiz_variance": 20,
        "pos_vert_variance": 30,
        "compass_variance": 40,
        "terrain_alt_variance": 50,
    },
    "VfrHud": {
        "airspeed": 0.0,
        "groundspeed": 1.5,
        "heading": 90,
        "throttle": 50,
        "alt": 5.5,
        "climb": 0.25,
    },
    "GlobalPositionInt": {
        "time_boot_ms": 123456,
        "lat": 311234567,
        "lon": 121456789,
        "alt": 1000,
        "relative_alt": -5000,
        "vx": 50,
        "vy": -50,
        "vz": 25,
        "hdg": 9000,
    },
    "GpsRawInt": {
        "time_usec": 1234567890,
        "fix_type": 3,
        "lat": 311234567,
        "lon": 121456789,
        "alt": 1000,
        "eph": 100,
        "epv": 200,
        "vel": 30,
        "cog": 9000,
        "satellites_visible": 12,
        "h_acc": 250,
        "v_acc": 400,
        "vel_acc": 300,
        "hdg_acc": 500,
    },
    "WaterDepth": {
        "time_boot_ms": 123456,
        "id": 0,
        "healthy": 1,
        "lat": 311234567,
        "lng": 121456789,
        "altitude": 5.0,
        "bottom_distance": 25.0,
        "terrain_height": -20.5,
        "temperature": 15.5,
    },
    "DistanceSensor": {
        "time_boot_ms": 123456,
        "min_distance": 20,
        "max_distance": 5000,
        "current_distance": 250,
        "type": 1,
        "id": 0,
        "orientation": 0,
        "covariance": 255,
        "horizontal_fov": 0.5,
        "vertical_fov": 0.25,
        "signal_quality": 100,
    },
    "ManualControl": {
        "sequence": 7,
        "x": 600,
        "y": -300,
        "z": 125,
        "p": 0,
        "r": 0,
        "yaw": -100,
    },
    "Command": {
        "command": 1,       # CMD_SET_MODE
        "param": 3,         # HOLD
    },
    "RcChannels": {
        "time_boot_ms": 123456,
        "chancount": 6,
        "chan_raw": [1500, 1500, 1500, 1500, 1100, 1900] + [65535] * 12,
        "rssi": 100,
    },
    "ServoOutputRaw": {
        "time_boot_ms": 123456,
        "port": 0,
        "servo_raw": [1500, 1600, 1400, 1500, 1100, 1900, 1500, 1500] + [1500] * 8,
    },
    "ParamSet": {
        "param_id": b"P_GAIN\0" + b"\0" * 9,
        "param_value": 0.5,
        "param_type": 2,
    },
    "ParamValue": {
        "param_id": b"P_GAIN\0" + b"\0" * 9,
        "param_value": 0.5,
        "param_type": 2,
        "param_count": 128,
        "param_index": 42,
    },
}


def crc16(data: bytes) -> int:
    """CRC-16/MODBUS matching src/protocol.cpp (init 0xFFFF, reflected poly 0xA001)."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def field_value(name: str, ftype: str, count: int) -> list:
    """取固定输入值，缺省补 0；char 数组按字节串补齐。"""
    default = FRAME_INPUTS.get(_frame_now, {}).get(name, 0)
    if ftype == "char":
        raw = bytes(default) if isinstance(default, (bytes, bytearray)) else str(default).encode()
        raw = raw[:count]
        return list(raw + b"\0" * (count - len(raw)))
    if count > 1:
        vals = list(default) if isinstance(default, (list, tuple)) else [default] * count
        if len(vals) != count:
            raise ValueError(f"{_frame_now}.{name} expects {count} values, got {len(vals)}")
        return vals
    return [default]


def build_payload(fields_layout: list[dict]) -> bytes:
    out = bytearray()
    for f in fields_layout:
        vals = field_value(f["name"], f["type"], f["count"])
        fmt = STRUCT_FMT[f["type"]]
        if fmt == "s":
            out.extend(struct.pack(f"<{f['count']}s", bytes(vals)))
        elif f["count"] > 1:
            out.extend(struct.pack(f"<{f['count']}{fmt}", *vals))
        else:
            out.extend(struct.pack(f"<{fmt}", vals[0]))
    return bytes(out)


def build_frame(payload: bytes, frame_type: int, version: int) -> bytes:
    """Assemble frame and append CRC over ver..payload."""
    length = len(payload)
    header = bytes([0xAA, 0x55, version, length, frame_type])
    crc = crc16(bytes([version, length, frame_type]) + payload)
    return header + payload + bytes([crc & 0xFF, (crc >> 8) & 0xFF])


_frame_now = ""


def make_cases(schema: dict) -> list[dict]:
    global _frame_now
    version = int(str(schema["version"]), 0)
    cases = []
    for frame_name, frame in schema["frames"].items():
        _frame_now = frame_name
        layout = compute_layout(frame["fields"])
        payload = build_payload(layout)
        frame_type = int(str(frame["type"]), 0)
        golden_frame = build_frame(payload, frame_type, version)
        inputs = {}
        for f in layout:
            vals = field_value(f["name"], f["type"], f["count"])
            if f["type"] == "char":
                inputs[f["name"]] = bytes(vals).split(b"\0")[0].decode("ascii", "replace")
            else:
                inputs[f["name"]] = vals if f["count"] > 1 else vals[0]
        cases.append({
            "name": frame_name,
            "array_name": f"OU_GOLDEN_{frame_name.upper()}_FRAME",
            "frame_type": frame_type,
            "payload_size": len(payload),
            "total_length": len(golden_frame),
            "notes": frame.get("notes", ""),
            "inputs": inputs,
            "expected_frame": list(golden_frame),
        })
    return cases


def fmt_hex_byte(b: int) -> str:
    return f"0x{b:02X}"


def render_h(cases: list[dict]) -> str:
    lines = []
    lines.append(f"/* {AUTO_HEADER} */")
    lines.append("#ifndef OU_GOLDEN_H")
    lines.append("#define OU_GOLDEN_H")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("#define OU_GOLDEN_CRC_KNOWN_ANSWER 0x4B37")
    lines.append("")
    lines.append('/* CRC standard test vector: crc16("123456789") = 0x4B37 */')
    crc_data = list("123456789".encode("ascii"))
    crc_data_str = ", ".join(fmt_hex_byte(b) for b in crc_data)
    lines.append(f"static const uint8_t OU_GOLDEN_CRC_DATA_123456789[] = {{{crc_data_str}}};")
    lines.append("")
    for case in cases:
        name = case["name"]
        arr = case["array_name"]
        length = case["total_length"]
        lines.append(f"/* {name} (type=0x{case['frame_type']:02X}): {case['notes']} */")
        lines.append(f"#define {arr}_LEN {length}")
        bytes_str = ", ".join(fmt_hex_byte(b) for b in case["expected_frame"])
        lines.append(f"static const uint8_t {arr}[{arr}_LEN] = {{{bytes_str}}};")
        lines.append("")
    lines.append("#endif /* OU_GOLDEN_H */")
    return "\n".join(lines) + "\n"


def render_json(cases: list[dict]) -> str:
    payload = {
        "auto_header": AUTO_HEADER,
        "crc_known_answer": {"input": "123456789", "expected": "0x4B37"},
        "cases": cases,
    }
    return json.dumps(payload, indent=2, ensure_ascii=False) + "\n"


def write_if_changed(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return
    path.write_text(content, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate golden vectors from schema.")
    parser.add_argument("--schema", type=Path, default=SCHEMA_PATH)
    parser.add_argument("--out-dir", type=Path, default=OUT_DIR)
    args = parser.parse_args()

    schema = load_schema(args.schema)
    cases = make_cases(schema)
    out = args.out_dir

    write_if_changed(out / "golden.h", render_h(cases))
    write_if_changed(out / "golden.json", render_json(cases))

    for case in cases:
        print(
            f"{case['name']}: {case['total_length']} bytes, "
            f"payload {case['payload_size']} bytes, type=0x{case['frame_type']:02X}"
        )
    print(f"Generated golden vectors in {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
