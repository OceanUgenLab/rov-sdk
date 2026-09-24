<!-- AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml -->

# OU 通信协议（v0x03）

> 本文件由 `tools/codegen.py` 从 `schema/protocol.yaml` 自动生成。三端（上位机 SDK、算力板、STM32 固件）以 `schema/protocol.yaml` 为唯一权威源。

## 1. 帧格式

```
AA 55 | ver(1B) | len(1B) | type(1B) | payload(len) | crc16(2B, 小端)
```

| 字段 | 长度 | 说明 |
|------|------|------|
| STX0 | 1B | 固定 `0xAA` |
| STX1 | 1B | 固定 `0x55` |
| ver | 1B | 协议版本，固定 `0x03` |
| len | 1B | payload 字节数（不含帧头、不含 CRC） |
| type | 1B | 帧类型（见帧清单） |
| payload | len | 载荷（见下） |
| crc16 | 2B | CRC-16/MODBUS，小端；覆盖范围从 `ver` 起共 `3 + len` 字节（不含 STX） |

- 无 ETX 结束符。帧总长 = `7 + len`。
- **字节序固定小端**（与 STM32 一致），浮点按 IEEE-754 float32 位模式传输。

## 2. CRC-16/MODBUS

- 多项式（反射形式）`0xA001`，初始值 `0xFFFF`，反射输入/输出，无 xorout。
- 标准校验向量：`crc16("123456789") == 0x4B37`。

## 3. 枚举

### Mode（u8）

| 常量 | 值 |
|------|----|
| `MODE_MANUAL` | 0 |
| `MODE_AUTO` | 1 |
| `MODE_RETURN` | 2 |
| `MODE_HOLD` | 3 |

### SystemState（u8）

| 常量 | 值 |
|------|----|
| `SYSTEMSTATE_UNINIT` | 0 |
| `SYSTEMSTATE_BOOT` | 1 |
| `SYSTEMSTATE_CALIBRATING` | 2 |
| `SYSTEMSTATE_STANDBY` | 3 |
| `SYSTEMSTATE_ARMED` | 4 |
| `SYSTEMSTATE_ACTIVE` | 5 |
| `SYSTEMSTATE_CRITICAL` | 6 |
| `SYSTEMSTATE_EMERGENCY` | 7 |

### CommandId（u16）

| 常量 | 值 |
|------|----|
| `COMMANDID_CMD_SET_MODE` | 1 |
| `COMMANDID_CMD_ARM` | 2 |
| `COMMANDID_CMD_GO_HOME` | 4 |
| `COMMANDID_CMD_SET_STREAM` | 5 |
| `COMMANDID_CMD_SET_PWM` | 6 |


## 4. 帧清单

| type | 帧 | 方向 | payload | 说明 |
|------|----|------|---------|------|
| `0x50` | Heartbeat | both | 4B | 心跳，双向 1 Hz；armed 判断 = state >= ARMED 且 < CRITICAL |
| `0x10` | SysStatus | up | 49B | 系统健康 + 电池 + 链路统计 + 遥测开关，1 Hz |
| `0x51` | CommandAck | up | 8B | 命令应答，与 Command 的 command 字段对号 |
| `0x13` | PoseNed | up | 52B | 姿态 + 本地 NED 位置/速度，10–50 Hz；姿态与位置共享时间戳 |
| `0x14` | EkfStatusReport | up | 7B | EKF 健康/方差，1 Hz；方差均为 ×100 编码 |
| `0x15` | VfrHud | up | 20B | 人工仪表量，1–5 Hz |
| `0x16` | GlobalPositionInt | up | 28B | 全局经纬高 + NED 速度，1 Hz |
| `0x18` | GpsRawInt | up | 46B | GPS 原始回显，默认关闭，由 CMD_SET_STREAM 开启 |
| `0x19` | WaterDepth | up | 30B | 水深/距底/水温（Sub 关键帧），1–10 Hz |
| `0x52` | DistanceSensor | both | 23B | 测距（避碰声呐），事件/1 Hz |
| `0x30` | ManualControl | down | 14B | 六轴速度/角速度指令，10–50 Hz；杆量 0 = 该轴交由飞控自稳；sequence 不递增或输入超时（约 500 ms）即 failsafe |
| `0x32` | Command | down | 6B | 命令帧；飞控以 CommandAck 应答，超时 0.5–1 s 重发 |
| `0x1A` | RcChannels | up | 42B | 遥控通道回显，默认关闭，由 CMD_SET_STREAM 开启 |
| `0x1B` | ServoOutputRaw | up | 37B | 执行器 PWM 输出回显，默认关闭，由 CMD_SET_STREAM 开启 |
| `0x33` | ParamSet | down | 21B | 写参数 |
| `0x1D` | ParamValue | up | 25B | 参数值回读（飞控实际保存值，非请求值回显） |

## 5. 载荷定义

### Heartbeat `type=0x50`，payload 4 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | mode | u8 | "" | {0: 'MANUAL', 1: 'AUTO', 2: 'RETURN', 3: 'HOLD'} | 当前运行模式（权威来源） |
| 1 | system_type | u8 | "" | [0, 255] | bit0 角色 0=机器人 1=地面站；bit1..7 产品序号 |
| 2 | fw_version | u8 | "" | [0, 255] | 发送方固件/SDK 主版本号 |
| 3 | system_state | u8 | "" | {0: 'UNINIT', 7: 'EMERGENCY'} | SystemState 枚举 |

### SysStatus `type=0x10`，payload 49 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | sensors_present | u32 | "" | [0, 4294967295] | 已安装传感器位图 |
| 4 | sensors_enabled | u32 | "" | [0, 4294967295] | 已启用传感器位图 |
| 8 | sensors_health | u32 | "" | [0, 4294967295] | 传感器健康位图 |
| 12 | load | u16 | %x10 | [0, 1000] | CPU 负载 %×10 |
| 14 | voltage_total | u16 | mV | [0, 65535] | 电池总电压，65535=未知 |
| 16 | voltage_cell_max | u16 | mV | [0, 65535] | 单体最高电压，65535=未知 |
| 18 | voltage_cell_min | u16 | mV | [0, 65535] | 单体最低电压，65535=未知 |
| 20 | current_battery | i16 | cA | [-1, 32767] | 电池电流，-1=未知 |
| 22 | battery_remaining | i8 | % | [-1, 100] | 剩余百分比，-1=未知 |
| 23 | current_consumed | i32 | mAh | [-1, 2147483647] | 已消耗容量，-1=未知 |
| 27 | battery_temperature | i16 | °Cx100 | [-32768, 32767] | 电池温度 °C×100 |
| 29 | battery_fault_bitmask | u32 | "" | [0, 4294967295] | 电池故障位图 |
| 33 | drop_rate_comm | u16 | %x100 | [0, 65535] | 丢包率 %×100 |
| 35 | errors_comm | u16 | "" | [0, 65535] | 通信错误计数 |
| 37 | errors_count[0..3] | u16×4 | "" | [0, 65535] | 系统错误计数 |
| 45 | stream_mask | u32 | "" | [0, 7] | 回显帧开关位图 bit0=GpsRawInt bit1=RcChannels bit2=ServoOutputRaw |

### CommandAck `type=0x51`，payload 8 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | command | u16 | "" | [1, 65535] | 被确认的命令号（Command 枚举） |
| 2 | result | u8 | "" | [0, 5] | 0 接受 1 暂时拒绝 2 拒绝 3 不支持 4 失败 5 进行中 |
| 3 | progress | u8 | % | [0, 100] | 进度百分比 |
| 4 | result_param2 | i32 | "" | [-2147483648, 2147483647] | 附加结果参数 |

### PoseNed `type=0x13`，payload 52 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | time_boot_ms | u32 | ms | [0, 4294967295] | 开机毫秒时间戳 |
| 4 | roll | f32 | rad | [-3.14159265, 3.14159265] | 横滚角 |
| 8 | pitch | f32 | rad | [-1.57079633, 1.57079633] | 俯仰角 |
| 12 | yaw | f32 | rad | [-3.14159265, 3.14159265] | 偏航角 |
| 16 | rollspeed | f32 | rad/s | [-10.0, 10.0] | 横滚角速度 |
| 20 | pitchspeed | f32 | rad/s | [-10.0, 10.0] | 俯仰角速度 |
| 24 | yawspeed | f32 | rad/s | [-10.0, 10.0] | 偏航角速度 |
| 28 | x | f32 | m | [-10000.0, 10000.0] | 北向位置（NED） |
| 32 | y | f32 | m | [-10000.0, 10000.0] | 东向位置（NED） |
| 36 | z | f32 | m | [-10000.0, 10000.0] | 下向位置（NED） |
| 40 | vx | f32 | m/s | [-10.0, 10.0] | 北向速度 |
| 44 | vy | f32 | m/s | [-10.0, 10.0] | 东向速度 |
| 48 | vz | f32 | m/s | [-10.0, 10.0] | 下向速度 |

### EkfStatusReport `type=0x14`，payload 7 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | flags | u16 | "" | [0, 65535] | EKF 健康/融合状态位 |
| 2 | velocity_variance | u8 | x100 | [0, 255] | 速度方差×100 |
| 3 | pos_horiz_variance | u8 | x100 | [0, 255] | 水平位置方差×100 |
| 4 | pos_vert_variance | u8 | x100 | [0, 255] | 垂直位置方差×100 |
| 5 | compass_variance | u8 | x100 | [0, 255] | 罗盘方差×100 |
| 6 | terrain_alt_variance | u8 | x100 | [0, 255] | 地形高度方差×100 |

### VfrHud `type=0x15`，payload 20 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | airspeed | f32 | m/s | [0.0, 10.0] | 水航速（无传感器填 0） |
| 4 | groundspeed | f32 | m/s | [0.0, 10.0] | 对地速度 |
| 8 | heading | i16 | deg | [0, 359] | 航向角 |
| 10 | throttle | u16 | % | [0, 100] | 油门档位 |
| 12 | alt | f32 | m | [0.0, 100.0] | 距底高度（与 WaterDepth.altitude 同义） |
| 16 | climb | f32 | m/s | [-5.0, 5.0] | 垂直速度，上浮为正 |

### GlobalPositionInt `type=0x16`，payload 28 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | time_boot_ms | u32 | ms | [0, 4294967295] | 开机毫秒时间戳 |
| 4 | lat | i32 | degE7 | [-900000000, 900000000] | 纬度×1e7 |
| 8 | lon | i32 | degE7 | [-1800000000, 1800000000] | 经度×1e7 |
| 12 | alt | i32 | mm | [-2147483648, 2147483647] | 海拔高度 AMSL |
| 16 | relative_alt | i32 | mm | [-2147483648, 2147483647] | 相对 Home 高度 |
| 20 | vx | i16 | cm/s | [-32768, 32767] | 北向速度 |
| 22 | vy | i16 | cm/s | [-32768, 32767] | 东向速度 |
| 24 | vz | i16 | cm/s | [-32768, 32767] | 下向速度 |
| 26 | hdg | u16 | cdeg | [0, 35999] | 航向×100，65535=未知 |

### GpsRawInt `type=0x18`，payload 46 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | time_usec | u64 | us | [0, 18446744073709551615] | us 级时间戳 |
| 8 | fix_type | u8 | "" | [0, 255] | 定位类型枚举 |
| 9 | lat | i32 | degE7 | [-900000000, 900000000] | 纬度×1e7 |
| 13 | lon | i32 | degE7 | [-1800000000, 1800000000] | 经度×1e7 |
| 17 | alt | i32 | mm | [-2147483648, 2147483647] | 海拔高度 |
| 21 | eph | u16 | cm | [0, 65535] | 水平精度×100，65535=未知 |
| 23 | epv | u16 | cm | [0, 65535] | 垂直精度×100，65535=未知 |
| 25 | vel | u16 | cm/s | [0, 65535] | 地速 |
| 27 | cog | u16 | cdeg | [0, 35999] | 航向×100 |
| 29 | satellites_visible | u8 | "" | [0, 255] | 可见卫星数 |
| 30 | h_acc | u32 | mm | [0, 4294967295] | 水平精度 |
| 34 | v_acc | u32 | mm | [0, 4294967295] | 垂直精度 |
| 38 | vel_acc | u32 | mm/s | [0, 4294967295] | 速度精度 |
| 42 | hdg_acc | u32 | cdegE-2 | [0, 4294967295] | 航向精度 |

### WaterDepth `type=0x19`，payload 30 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | time_boot_ms | u32 | ms | [0, 4294967295] | 开机毫秒时间戳 |
| 4 | id | u8 | "" | [0, 255] | 传感器 ID |
| 5 | healthy | u8 | "" | [0, 1] | 健康状态 |
| 6 | lat | i32 | degE7 | [-900000000, 900000000] | 纬度×1e7，可选 |
| 10 | lng | i32 | degE7 | [-1800000000, 1800000000] | 经度×1e7，可选 |
| 14 | altitude | f32 | m | [0.0, 100.0] | 距底高度 |
| 18 | bottom_distance | f32 | m | [0.0, 300.0] | 到水底距离 |
| 22 | terrain_height | f32 | m | [-300.0, 300.0] | 地形高度 |
| 26 | temperature | f32 | °C | [-5.0, 45.0] | 水温 |

### DistanceSensor `type=0x52`，payload 23 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | time_boot_ms | u32 | ms | [0, 4294967295] | 开机毫秒时间戳 |
| 4 | min_distance | u16 | cm | [0, 65535] | 量程下限 |
| 6 | max_distance | u16 | cm | [0, 65535] | 量程上限 |
| 8 | current_distance | u16 | cm | [0, 65535] | 当前距离 |
| 10 | type | u8 | "" | [0, 255] | 测距类型 |
| 11 | id | u8 | "" | [0, 255] | 传感器 ID |
| 12 | orientation | u8 | "" | [0, 255] | 朝向 |
| 13 | covariance | u8 | cm2 | [0, 255] | 协方差，255=未知 |
| 14 | horizontal_fov | f32 | rad | [0.0, 6.28318531] | 水平视场角 |
| 18 | vertical_fov | f32 | rad | [0.0, 6.28318531] | 垂直视场角 |
| 22 | signal_quality | u8 | "" | [0, 100] | 信号质量，255=未知 |

### ManualControl `type=0x30`，payload 14 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | sequence | u16 | "" | [0, 65535] | 递增序号，丢包/乱序检测 |
| 2 | x | i16 | "" | [-1000, 1000] | 前后速度 surge |
| 4 | y | i16 | "" | [-1000, 1000] | 横移速度 sway |
| 6 | z | i16 | "" | [-1000, 1000] | 升沉速度 heave |
| 8 | p | i16 | "" | [-1000, 1000] | 俯仰角速度 pitch rate |
| 10 | r | i16 | "" | [-1000, 1000] | 横滚角速度 roll rate |
| 12 | yaw | i16 | "" | [-1000, 1000] | 偏航角速度 yaw rate |

### Command `type=0x32`，payload 6 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | command | u16 | "" | [1, 65535] | Command 枚举 |
| 2 | param | u32 | "" | [0, 4294967295] | 命令参数，按命令定义解释 |

### RcChannels `type=0x1A`，payload 42 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | time_boot_ms | u32 | ms | [0, 4294967295] | 开机毫秒时间戳 |
| 4 | chancount | u8 | "" | [0, 18] | 有效通道数 |
| 5 | chan_raw[0..17] | u16×18 | us | [0, 65535] | 18 路通道原始值，65535=无效 |
| 41 | rssi | u8 | "" | [0, 100] | 遥控链路强度，255=未知 |

### ServoOutputRaw `type=0x1B`，payload 37 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | time_boot_ms | u32 | ms | [0, 4294967295] | ms 级时间戳 |
| 4 | port | u8 | "" | [0, 255] | 输出端口 |
| 5 | servo_raw[0..15] | u16×16 | us | [0, 65535] | 16 路 PWM 输出 |

### ParamSet `type=0x33`，payload 21 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | param_id[0..15] | char×16 | "" |  | 参数名 |
| 16 | param_value | f32 | "" | ['-3.4e38', '3.4e38'] | 目标值 |
| 20 | param_type | u8 | "" | [0, 255] | OU 类型枚举 |

### ParamValue `type=0x1D`，payload 25 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | param_id[0..15] | char×16 | "" |  | 参数名 |
| 16 | param_value | f32 | "" | ['-3.4e38', '3.4e38'] | 实际保存值 |
| 20 | param_type | u8 | "" | [0, 255] | OU 类型枚举 |
| 21 | param_count | u16 | "" | [0, 65535] | 参数总数 |
| 23 | param_index | u16 | "" | [0, 65535] | 当前序号 |


## 6. 实现约束

- 解析必须**滑动窗口**处理流（容忍噪声前缀/粘包），STX 不匹配时逐字节推进。
- `ver`/`len`/`type`/CRC 任一校验失败即丢弃该帧（跳到下一帧边界）。
- 载荷校验失败时**不修改**输出缓冲区。