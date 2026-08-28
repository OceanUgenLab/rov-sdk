<!-- AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml -->

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
- 标准校验向量：`crc16("123456789") == 0x4B37`。
- 覆盖范围：`ver`、`len`、`type`、`payload`，即从 `ver` 起共 `3 + len` 字节。

## 3. 模式枚举

| 常量 | 值 | 说明 |
|------|----|------|
| `MODE_MANUAL` | 0 | 手动遥控 |
| `MODE_AUTO` | 1 | 自动控制 |
| `MODE_RETURN` | 2 | 一键返航 |
| `MODE_HOLD` | 3 | 悬停保持 |

## 4. 载荷定义

### 4.1 控制指令 `type=0x01`（主控 -> 从控），payload 52 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | mode | uint8 | "" | {0: 'MANUAL', 1: 'AUTO', 2: 'RETURN', 3: 'HOLD'} | 运行模式 |
| 1 | armed | uint8 | "" | [0, 1] | 解锁标志，1=armed 输出推进器，0=停机 |
| 2 | reserved[0..1] | uint8×2 | "" | [0, 0] | 保留，发送方清零，接收方忽略 |
| 4 | surge | float32 | "" | [-1.0, 1.0] | 前(+) / 后(-) |
| 8 | sway | float32 | "" | [-1.0, 1.0] | 右平移(+) / 左平移(-) |
| 12 | heave | float32 | "" | [-1.0, 1.0] | 上浮(+) / 下潜(-) |
| 16 | yaw | float32 | "" | [-1.0, 1.0] | 右转(+) / 左转(-) |
| 20 | target_depth | float32 | m | [0.0, 300.0] | 目标深度 |
| 24 | target_heading | float32 | deg | [0.0, 360.0] | 目标航向 |
| 28 | target_north | float32 | m | [-10000.0, 10000.0] | v2, firmware v1 ignores（目标北向位移，寻迹/返航用） |
| 32 | target_east | float32 | m | [-10000.0, 10000.0] | v2, firmware v1 ignores（目标东向位移，寻迹/返航用） |
| 36 | reserved_tail[0..15] | uint8×16 | "" | [0, 0] | 扩展预留区（云台/机械臂/照明等后置），发送方清零，接收方忽略 |

### 4.2 遥测 `type=0x02`（从控 -> 主控），payload 128 字节

| 偏移 | 字段 | 类型 | 单位 | 范围 | 说明 |
|------|------|------|------|------|------|
| 0 | roll | float32 | deg | [-180.0, 180.0] | 横滚角 |
| 4 | pitch | float32 | deg | [-90.0, 90.0] | 俯仰角 |
| 8 | heading | float32 | deg | [0.0, 360.0] | 航向角 |
| 12 | yaw_rate | float32 | deg/s | [-180.0, 180.0] | 偏航角速度 |
| 16 | depth | float32 | m | [0.0, 300.0] | 水深 |
| 20 | altitude | float32 | m | [0.0, 100.0] | 距底高度 |
| 24 | north | float32 | m | [-10000.0, 10000.0] | 相对原点北向位移 |
| 28 | east | float32 | m | [-10000.0, 10000.0] | 相对原点东向位移 |
| 32 | vn | float32 | m/s | [-10.0, 10.0] | 北向速度 |
| 36 | ve | float32 | m/s | [-10.0, 10.0] | 东向速度 |
| 40 | vd | float32 | m/s | [-10.0, 10.0] | 下潜速度（向下为正） |
| 44 | voltage | float32 | V | [0.0, 60.0] | 电池电压 |
| 48 | current | float32 | A | [0.0, 100.0] | 电池电流 |
| 52 | percent | float32 | % | [0.0, 100.0] | 电池电量百分比 |
| 56 | temperature | float32 | °C | [-10.0, 60.0] | 电池/舱内温度 |
| 60 | water_temp | float32 | °C | [-5.0, 45.0] | 水温 |
| 64 | salinity | float32 | PSU | [0.0, 45.0] | 盐度 |
| 68 | pressure_bar | float32 | bar | [0.0, 40.0] | 水压 |
| 72 | cable_tension | float32 | N | [0.0, 1000.0] | 缆绳张力 |
| 76 | thr[0..7] | float32×8 | % | [-1.0, 1.0] | 8 路推进器输出百分比 |
| 108 | leak | uint8 | "" | [0, 1] | 漏水检测，1=报警 |
| 109 | armed | uint8 | "" | [0, 1] | 当前 armed 状态 |
| 110 | mode | uint8 | "" | {0: 'MANUAL', 1: 'AUTO', 2: 'RETURN', 3: 'HOLD'} | 当前运行模式 |
| 111 | reserved | uint8 | "" | [0, 0] | 保留，发送方清零，接收方忽略 |
| 112 | reserved_tail[0..15] | uint8×16 | "" | [0, 0] | 扩展预留区（云台/机械臂/照明等后置），发送方清零，接收方忽略 |

## 5. 实现约束

- 解析必须**滑动窗口**处理流（容忍噪声前缀/粘包），STX 不匹配时逐字节推进。
- `ver`/`len`/`type`/CRC 任一校验失败即丢弃该帧（跳到下一帧边界）。
- 载荷校验失败时**不修改**输出缓冲区。