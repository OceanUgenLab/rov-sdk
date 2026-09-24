# 06 电池和能源

## BATTERY_STATUS（飞控→上位机，P0）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `id` | `uint8` | 电池 ID |
| `battery_function` | `uint8` | 电池用途 |
| `type` | `uint8` | 化学类型 |
| `temperature` | `int16` | °C×100 |
| `voltages[10]` | `uint16[10]` | mV，65535 未知 |
| `current_battery` | `int16` | cA，-1 未知 |
| `current_consumed` | `int32` | mAh，-1 未知 |
| `energy_consumed` | `int32` | hJ，-1 未知 |
| `battery_remaining` | `int8` | %，-1 未知 |
| `time_remaining` | `int32` | s，-1 未知 |
| `charge_state` | `uint8` | 充电状态 |
| `voltages_ext[4]` | `uint16[4]` | mV |
| `mode` / `fault_bitmask` | `uint8` / `uint32` | 电池模式和故障位 |

OU 首版至少保留总电压、单体最高/最低电压、电流、剩余百分比、温度、消耗容量和故障位。

## BATTERY2（飞控→上位机，P1）

| 字段 | 类型 | 单位 |
|---|---|---|
| `voltage` | `uint16` | mV |
| `current_battery` | `int16` | cA |
| `temperature` | `int16` | °C×100 |
| `battery_remaining` | `int8` | % |

## POWER_STATUS（飞控→上位机，P1）

| 字段 | 类型 | 说明 |
|---|---|---|
| `Vcc` | `uint16` | mV |
| `Vservo` | `uint16` | mV |
| `Vcc` / `Vservo` 状态位 | `uint16` | 电源故障位图 |

