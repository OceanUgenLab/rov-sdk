# 05 遥控器、执行器和电机

## RC_CHANNELS（飞控→上位机，P0）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `chancount` | `uint8` | 有效通道数 |
| `chan1_raw..chan18_raw` | `uint16` | us，65535 无效 |
| `rssi` | `uint8` | 0..100，255 未知 |

## RC_CHANNELS_SCALED（飞控→上位机，P1）

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `chan1_scaled..chan8_scaled` | `int16` | -10000..10000 |
| `port` | `uint8` | 端口 |
| `rssi` | `uint8` | 0..100 |

## SERVO_OUTPUT_RAW（飞控→上位机，P0）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_usec` | `uint32/uint64` | us |
| `port` | `uint8` | 输出端口 |
| `servo1_raw..servo16_raw` | `uint16` | PWM us |

## RPM（飞控→上位机，P1）

| 字段 | 类型 | 单位 |
|---|---|---|
| `timestamp` | `uint64` | us |
| `frequency` | `float[4]` | Hz |
| `rpm` | `float[4]` | rpm |

## ESC_TELEMETRY_1_TO_4（飞控→上位机，P2）

| 字段 | 类型 | 单位 |
|---|---|---|
| `temperature[4]` | `int16[4]` | °C |
| `voltage[4]` | `uint16[4]` | V×100 |
| `current[4]` | `uint16[4]` | A×100 |
| `totalcurrent[4]` | `uint16[4]` | Ah×100 |
| `rpm[4]` | `uint16[4]` | rpm |
| `count` | `uint8` | 有效 ESC 数 |

