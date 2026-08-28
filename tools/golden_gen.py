#!/usr/bin/env python3
"""rovi_sdk schema-driven golden vector generator.

Reads schema/protocol.yaml and emits:
    generated/golden/golden.h   (C-compatible byte arrays)
    generated/golden/golden.json (human/tool-readable cases)

All generated files carry the header:
    AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

Frame layout (v0.2.0): AA 55 | ver(1B) | len(1B) | type(1B) | payload | crc16(2B LE)
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

# Fixed test inputs: every float is chosen to be exactly representable in IEEE-754 float32.
CMD_INPUTS = {
    "mode": 1,  # AUTO
    "armed": 1,
    "reserved": [0, 0],
    "surge": 0.5,
    "sway": -0.25,
    "heave": 1.0,
    "yaw": 0.125,
    "target_depth": 10.0,
    "target_heading": 90.0,
    "target_north": 0.0,
    "target_east": 0.0,
    "reserved_tail": [0] * 16,
}

TELE_INPUTS = {
    "roll": 0.0,
    "pitch": -0.25,
    "heading": 90.0,
    "yaw_rate": 0.125,
    "depth": 12.5,
    "altitude": 5.0,
    "north": -0.25,
    "east": 0.25,
    "vn": 0.5,
    "ve": -0.5,
    "vd": 0.125,
    "voltage": 25.0,
    "current": 2.5,
    "percent": 75.0,
    "temperature": 25.0,
    "water_temp": 5.0,
    "salinity": 35.0,
    "pressure_bar": 10.0,
    "cable_tension": 250.0,
    "thr": [0.5, -0.25, 1.0, -1.0, 0.125, -0.5, 0.75, -0.75],
    "leak": 0,
    "armed": 1,
    "mode": 1,  # AUTO
    "reserved": 0,
    "reserved_tail": [0] * 16,
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


def build_payload(fields_layout: list[dict], values: dict) -> bytes:
    """Encode payload bytes from schema field order and fixed values."""
    out = bytearray()
    for f in fields_layout:
        name = f["name"]
        count = f["count"]
        if f["type"] == "u8":
            raw = values[name]
            if count > 1:
                if len(raw) != count:
                    raise ValueError(f"u8 array {name} expects {count} values, got {len(raw)}")
                out.extend(struct.pack(f"<{count}B", *raw))
            else:
                out.append(int(raw) & 0xFF)
        elif f["type"] == "f32":
            raw = values[name]
            if count > 1:
                if len(raw) != count:
                    raise ValueError(f"f32 array {name} expects {count} values, got {len(raw)}")
                out.extend(struct.pack(f"<{count}f", *raw))
            else:
                out.extend(struct.pack("<f", float(raw)))
        else:
            raise ValueError(f"unknown type {f['type']}")
    return bytes(out)


def build_frame(payload: bytes, frame_type: int, version: int = 0x02) -> bytes:
    """Assemble v0.2.0 frame and append CRC over ver..payload."""
    stx0 = 0xAA
    stx1 = 0x55
    length = len(payload)
    header = bytes([stx0, stx1, version, length, frame_type])
    crc_body = bytes([version, length, frame_type]) + payload
    crc = crc16(crc_body)
    return header + payload + bytes([crc & 0xFF, (crc >> 8) & 0xFF])


def case_from_inputs(
    schema: dict,
    frame_name: str,
    inputs: dict,
    array_name: str,
    notes: str,
) -> dict:
    fields = compute_layout(schema["frames"][frame_name]["fields"])
    payload = build_payload(fields, inputs)
    frame_type = schema["frames"][frame_name]["type"]
    frame = build_frame(payload, frame_type)
    return {
        "name": frame_name,
        "array_name": array_name,
        "frame_type": frame_type,
        "payload_size": len(payload),
        "total_length": len(frame),
        "notes": notes,
        "inputs": inputs,
        "expected_frame": list(frame),
    }


def make_cases(schema: dict) -> list[dict]:
    cases = [
        case_from_inputs(
            schema,
            "CmdPacket",
            CMD_INPUTS,
            "ROVI_GOLDEN_CMD_FRAME",
            "固定控制指令 golden 帧，mode=AUTO, armed=1, 全部 reserved 清零",
        ),
        case_from_inputs(
            schema,
            "TelemetryPacket",
            TELE_INPUTS,
            "ROVI_GOLDEN_TELE_FRAME",
            "固定遥测 golden 帧，mode=AUTO, armed=1, 8 路 thr 互异，全部 reserved 清零",
        ),
    ]
    return cases


def fmt_hex_byte(b: int) -> str:
    return f"0x{b:02X}"


def render_h(cases: list[dict]) -> str:
    lines = []
    lines.append(f"/* {AUTO_HEADER} */")
    lines.append("#ifndef ROVI_GOLDEN_H")
    lines.append("#define ROVI_GOLDEN_H")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append(f"#define ROVI_GOLDEN_CRC_KNOWN_ANSWER 0x4B37")
    lines.append("")
    lines.append("/* CRC standard test vector: crc16(\"123456789\") = 0x4B37 */")
    crc_data = list("123456789".encode("ascii"))
    crc_data_str = ", ".join(fmt_hex_byte(b) for b in crc_data)
    lines.append(f"static const uint8_t ROVI_GOLDEN_CRC_DATA_123456789[] = {{{crc_data_str}}};")
    lines.append("")
    for case in cases:
        name = case["name"]
        arr = case["array_name"]
        length = case["total_length"]
        lines.append(f"/* {name}: {case['notes']} */")
        lines.append(f"#define {arr}_LEN {length}")
        bytes_str = ", ".join(fmt_hex_byte(b) for b in case["expected_frame"])
        lines.append(f"static const uint8_t {arr}[{arr}_LEN] = {{{bytes_str}}};")
        lines.append("")
    lines.append("#endif /* ROVI_GOLDEN_H */")
    return "\n".join(lines) + "\n"


def render_json(cases: list[dict]) -> str:
    # Keep readable with indentation; ensure deterministic ordering.
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
