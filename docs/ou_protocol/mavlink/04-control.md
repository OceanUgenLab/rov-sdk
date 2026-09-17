# 04 控制输入和模式控制

## MANUAL_CONTROL（上位机→飞控，P0）

| 字段 | 类型 | 单位/取值 |
|---|---|---|
| `x` | `int16` | 前后，-1000..1000 |
| `y` | `int16` | 横移，-1000..1000 |
| `z` | `int16` | 升沉/油门，通常 0..1000 |
| `r` | `int16` | 偏航，-1000..1000 |
| `buttons` | `uint16` | 按键位图 |
| `target` | `uint8` | 目标系统 |
| `enabled_extensions` | `uint8` | 扩展字段有效位 |
| `s` / `t` / `aux1..aux6` | `int16` | 扩展控制轴 |

OU 必须增加 `sequence`、`timestamp_ms` 和 `valid_mask`。不应把缺失通道误当作 0。

## RC_CHANNELS_OVERRIDE（上位机→飞控，P1）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `chan1_raw..chan18_raw` | `uint16` | us；65535 忽略，0 释放 |
| `target_system` | `uint8` | 目标系统 |
| `target_component` | `uint8` | 目标组件 |

## SET_MODE（上位机→飞控，P0）

| 字段 | 类型 | 说明 |
|---|---|---|
| `base_mode` | `uint8` | 基础模式位 |
| `custom_mode` | `uint32` | ArduSub 模式号 |
| `target_system` | `uint8` | 目标系统 |

## SET_ATTITUDE_TARGET（上位机→飞控，P1）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `q` | `float[4]` | 目标姿态四元数 |
| `body_roll_rate` / `body_pitch_rate` / `body_yaw_rate` | `float` | rad/s |
| `thrust` | `float` | 0..1 |
| `type_mask` | `uint8` | 忽略字段位图 |
| `target_system` / `target_component` | `uint8` | 目标 |

## SET_POSITION_TARGET_LOCAL_NED（上位机→飞控，P1）

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `x` / `y` / `z` | `float` | m，NED |
| `vx` / `vy` / `vz` | `float` | m/s |
| `afx` / `afy` / `afz` | `float` | m/s² |
| `yaw` | `float` | rad |
| `yaw_rate` | `float` | rad/s |
| `coordinate_frame` | `uint8` | 坐标系 |
| `type_mask` | `uint16` | 字段忽略位图 |

## COMMAND_LONG（上位机→飞控，P0）

| 字段 | 类型 | 说明 |
|---|---|---|
| `param1..param7` | `float` | 命令参数 |
| `command` | `uint16` | `MAV_CMD` |
| `target_system` / `target_component` | `uint8` | 目标 |
| `confirmation` | `uint8` | 重发确认次数 |

OU 不建议直接传 7 个无名字浮点数；应为每个高频命令定义明确字段，同时保留 `command` 和 ACK 语义。

