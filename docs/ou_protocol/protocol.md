# OU 协议 v0.3（P0）总览

> 本文是 OU 协议 P0 阶段的完整介绍。`schema/protocol.yaml` 仍为唯一真源，
> 本文是写入 schema 前的评审定稿。

## 1. 链路模型

点对点链路（一根缆 / 一个 UDP 对端），两端角色：

- **机器人侧**（飞控/载具）：发送遥测，执行命令；
- **地面站侧**（上位机 SDK）：发送控制与命令，消费遥测。

无帧内寻址字段——所有 `target_system` 类字段一律不使用。

## 2. 帧格式

所有帧共用统一封套：

```
| STX (0xAA 0x55) | ver (0x02) | len (u8) | type (u8) | payload (0..255B) | crc16 (le) |
```

| 项 | 定义 |
|---|---|
| 字节序 | 小端 |
| 浮点 | IEEE-754 float32 |
| len | payload 字节数，不含帧头与 CRC |
| CRC | CRC-16/MODBUS（poly 0xA001, init 0xFFFF），覆盖 ver 起共 3+len 字节 |

type 号段：`0x01` CmdPacket、`0x02` TelemetryPacket **均已废弃**（三端一次切换到 P0 帧，两号均不回收）；`0x10..0x2F` 上行（飞→站）；`0x30..0x4F` 下行（站→飞）；`0x50..0x5F` 双向。

## 3. 公共枚举与编码

### Mode（u8）

| 值 | 名称 | 含义 |
|---|------|------|
| 0 | MANUAL | 手动遥控 |
| 1 | AUTO | 自动任务 |
| 2 | RETURN | 一键返航 |
| 3 | HOLD | 定点保持 |

### SystemState（u8）

生命周期与安全状态，armed 判断：`state ≥ ARMED 且 < CRITICAL`。

| 值 | 名称 | 含义 |
|---|------|------|
| 0 | UNINIT | 上电自检中 |
| 1 | BOOT | 引导/系统初始化 |
| 2 | CALIBRATING | 校准中，禁止解锁 |
| 3 | STANDBY | 就绪，未解锁 |
| 4 | ARMED | 已解锁，推进器可出力 |
| 5 | ACTIVE | 作业中 |
| 6 | CRITICAL | 严重故障，可挽救 |
| 7 | EMERGENCY | 紧急（漏水/失控），执行安全行为 |
| 8..255 | 保留 | 接收方忽略 |

### SystemType（u8，位域）

| 位 | 含义 |
|---|------|
| bit0 | 角色：0=机器人，1=地面站 |
| bit1..7 | 产品序号：0、1 两种机型；未知值忽略 |

地面站侧产品序号填 0。

### Command（u16）

| 值 | 命令 | param（u32）含义 |
|---|------|------------------|
| 0x0001 | CMD_SET_MODE | 高 16 位=0；低 16 位=Mode 枚举值 |
| 0x0002 | CMD_ARM | 低 16 位：0=上锁（DISARM），1=解锁（ARM）；高 16 位=0 |
| 0x0003 | 保留 | （原 CMD_DISARM，并入 CMD_ARM param=0） |
| 0x0004 | CMD_GO_HOME | 0（预留） |
| 0x0005 | CMD_SET_STREAM | 高 16 位=目标帧 type；低 16 位=0 关 / 1 开 |
| 0x0006 | CMD_SET_PWM | 高 16 位=辅助通道号 1..16；低 16 位=PWM us（1100..1900） |

`CMD_SET_STREAM` 控制的回显帧（GPS_RAW_INT / RC_CHANNELS / SERVO_OUTPUT_RAW）
默认关闭，开关仅存飞控运行内存，重启后恢复全关；实际生效状态由
SYS_STATUS 的 `stream_mask` 持续广播。`CMD_SET_PWM` 仅作用于推进器混控之外的
辅助输出通道（云台/灯光/机械臂等），不参与混控。

## 4. 帧清单（18 帧）

| type | 帧 | 方向 | 默认 | 周期建议 | 用途 |
|------|----|------|------|---------|------|
| 0x50 | HEARTBEAT | 双向 | 开 | 1 Hz | 保活、模式与状态 |
| 0x10 | SYS_STATUS | 飞→站 | 开 | 1 Hz | 系统健康 + 电池 + 链路统计 + 遥测开关 |
| 0x11 | EXTENDED_SYS_STATE | 飞→站 | — | — | 占号保留，P0 不实现 |
| 0x51 | COMMAND_ACK | 飞→站 | 开 | 事件 | 命令应答 |
| 0x12 | STATUSTEXT | 飞→站 | — | — | 占号保留，P0 不实现 |
| 0x13 | POSE_NED | 飞→站 | 开 | 10–50 Hz | 姿态 + 本地 NED 位置/速度 |
| 0x14 | EKF_STATUS_REPORT | 飞→站 | 开 | 1 Hz | EKF 健康/方差 |
| 0x15 | VFR_HUD | 飞→站 | 开 | 1–5 Hz | 仪表量：速度/航向/油门/高度/爬升率 |
| 0x16 | GLOBAL_POSITION_INT | 飞→站 | 开 | 1 Hz | 全局经纬高 + NED 速度 |
| 0x18 | GPS_RAW_INT | 飞→站 | **关** | 1 Hz | GPS 原始回显（CMD_SET_STREAM 开启） |
| 0x19 | WATER_DEPTH | 飞→站 | 开 | 1–10 Hz | 水深/距底/水温 |
| 0x52 | DISTANCE_SENSOR | 双向 | 开 | 事件/1 Hz | 测距（避碰声呐） |
| 0x30 | MANUAL_CONTROL | 站→飞 | 开 | 10–50 Hz | 手动操控轴 |
| 0x32 | COMMAND | 站→飞 | 开 | 事件 | 命令通道（模式/解锁/遥测开关/PWM） |
| 0x1A | RC_CHANNELS | 飞→站 | **关** | 1 Hz | 遥控通道回显（CMD_SET_STREAM 开启） |
| 0x1B | SERVO_OUTPUT_RAW | 飞→站 | **关** | 1–10 Hz | 执行器 PWM 输出回显（CMD_SET_STREAM 开启） |
| 0x33 | PARAM_SET | 站→飞 | 开 | 事件 | 写参数 |
| 0x1D | PARAM_VALUE | 飞→站 | 开 | 事件 | 参数值回读 |

## 5. 载荷定义

### HEARTBEAT（0x50，双向，1 Hz，4 B）

| 字段 | 类型 | 说明 |
|---|---|---|
| mode | u8 | Mode 枚举，当前运行模式（权威来源） |
| system_type | u8 | SystemType 位域 |
| fw_version | u8 | 发送方固件/SDK 主版本号 |
| system_state | u8 | SystemState 枚举 |

任一端连续 3 个周期未收到对方心跳即判定链路断开，机器人侧触发失控保护。
完整版本号查询走 AUTOPILOT_VERSION（P1）。

### SYS_STATUS（0x10，飞→站，1 Hz）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| sensors_present | u32 | 已安装传感器位图 |
| sensors_enabled | u32 | 已启用传感器位图 |
| sensors_health | u32 | 传感器健康位图 |
| load | u16 | CPU 负载 %×10 |
| voltage_total | u16 | 电池总电压 mV，65535=未知 |
| voltage_cell_max / voltage_cell_min | u16 ×2 | 单体最高/最低 mV，65535=未知 |
| current_battery | i16 | cA，-1=未知 |
| battery_remaining | i8 | %，-1=未知 |
| current_consumed | i32 | mAh，-1=未知 |
| battery_temperature | i16 | °C×100 |
| battery_fault_bitmask | u32 | 电池故障位图 |
| drop_rate_comm | u16 | 丢包率 %×100 |
| errors_comm | u16 | 通信错误计数 |
| errors_count1..4 | u16 ×4 | 系统错误计数 |
| stream_mask | u32 | 回显帧开关位图：bit0=GPS_RAW_INT，bit1=RC_CHANNELS，bit2=SERVO_OUTPUT_RAW，1=发送中，其余位保留为 0 |

单电池模型；多电池等扩展需求出现时再引入独立帧。

### COMMAND_ACK（0x51，飞→站，事件）

| 字段 | 类型 | 说明 |
|---|---|---|
| command | u16 | 被确认的命令号（Command 枚举） |
| result | u8 | 接受 / 暂时拒绝 / 拒绝 / 不支持 / 失败 / 进行中 |
| progress | u8 | 进度 %（长任务） |
| result_param2 | i32 | 附加结果参数 |

### POSE_NED（0x13，飞→站，10–50 Hz，52 B）

| 字段 | 类型 | 单位 |
|---|---|---|
| time_boot_ms | u32 | ms |
| roll / pitch / yaw | f32 ×3 | rad，yaw ∈ -π..π |
| rollspeed / pitchspeed / yawspeed | f32 ×3 | rad/s |
| x / y / z | f32 ×3 | m，NED |
| vx / vy / vz | f32 ×3 | m/s，NED |

姿态与位置共享同一时间戳，供控制/导航算法消费；载荷 52 字节 = u32 时间戳 + 12 个 f32。

### EKF_STATUS_REPORT（0x14，飞→站，1 Hz）

| 字段 | 类型 | 说明 |
|---|---|---|
| flags | u16 | EKF 健康/融合状态位 |
| velocity_variance | u8 | ×100 |
| pos_horiz_variance | u8 | ×100 |
| pos_vert_variance | u8 | ×100 |
| compass_variance | u8 | ×100 |
| terrain_alt_variance | u8 | ×100 |

### VFR_HUD（0x15，飞→站，1–5 Hz）

| 字段 | 类型 | 单位 |
|---|---|---|
| airspeed / groundspeed | f32 ×2 | m/s |
| heading | i16 | deg |
| throttle | u16 | % |
| alt | f32 | m（距底高度，与 WATER_DEPTH.altitude 同义） |
| climb | f32 | m/s |

面向人工仪表显示。

### GLOBAL_POSITION_INT（0x16，飞→站，1 Hz）

| 字段 | 类型 | 单位 |
|---|---|---|
| time_boot_ms | u32 | ms |
| lat / lon | i32 ×2 | deg×1e7 |
| alt | i32 | mm，AMSL |
| relative_alt | i32 | mm，相对 Home |
| vx / vy / vz | i16 ×3 | cm/s，NED |
| hdg | u16 | cdeg，65535=未知 |

### GPS_RAW_INT（0x18，飞→站，默认关，1 Hz）

| 字段 | 类型 | 单位 |
|---|---|---|
| time_usec | u64 | us |
| fix_type | u8 | 定位类型枚举 |
| lat / lon | i32 ×2 | deg×1e7 |
| alt | i32 | mm |
| eph / epv | u16 ×2 | cm×100，65535=未知 |
| vel | u16 | cm/s |
| cog | u16 | cdeg |
| satellites_visible | u8 | 颗 |
| h_acc / v_acc / vel_acc / hdg_acc | u32 ×4 | mm / cdeg×100 |

### WATER_DEPTH（0x19，飞→站，1–10 Hz）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| time_boot_ms | u32 | ms |
| id | u8 | 传感器 ID |
| healthy | u8 | 健康状态 |
| lat / lng | i32 ×2 | deg×1e7，可选 |
| altitude | f32 | 距底高度 m |
| bottom_distance | f32 | 到水底距离 m |
| terrain_height | f32 | 地形高度 m |
| temperature | f32 | °C |

### DISTANCE_SENSOR（0x52，双向，事件/1 Hz）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| time_boot_ms | u32 | ms |
| min_distance / max_distance | u16 ×2 | cm |
| current_distance | u16 | cm |
| type | u8 | 测距类型 |
| id | u8 | 传感器 ID |
| orientation | u8 | 朝向 |
| covariance | u8 | cm²，255=未知 |
| horizontal_fov / vertical_fov | f32 ×2 | rad |
| signal_quality | u8 | 0..100，255=未知 |

外部注入的传感数据必须带源时间戳与质量；输入超时或质量无效时飞控停止融合。

### MANUAL_CONTROL（0x30，站→飞，10–50 Hz，14 B）

| 字段 | 类型 | 单位/取值 |
|---|---|---|
| sequence | u16 | 递增序号，用于丢包/乱序检测 |
| x | i16 | 前后速度（surge）-1000..1000 |
| y | i16 | 横移速度（sway）-1000..1000 |
| z | i16 | 升沉速度（heave）-1000..1000 |
| p | i16 | 俯仰角速度（pitch rate）-1000..1000 |
| r | i16 | 横滚角速度（roll rate）-1000..1000 |
| yaw | i16 | 偏航角速度（yaw rate）-1000..1000 |

六轴均为**速度/角速度指令**，归一化 -1000..1000。填 0 表示该轴交由飞控自稳：
在带增稳的模式下，杆量为 0 的轴由飞控闭环保持（定深、定向、水平等），
不存在"通道缺失"问题。失控保护：飞控检测 sequence 不递增或输入超时
（建议 500 ms）即进入 failsafe。

### COMMAND（0x32，站→飞，事件，6 B）

| 字段 | 类型 | 说明 |
|---|---|---|
| command | u16 | Command 枚举 |
| param | u32 | 按命令定义解释 |

命令闭环：COMMAND → COMMAND_ACK（`command` 对号应答）；上位机超时（0.5–1 s）
未收到 ACK 应重发，重试耗尽报命令失败。模式切换成功由下一帧 HEARTBEAT 的
`mode` 确认。`result` 区分「暂时拒绝（可重试）」与「拒绝（不应重试）」。

### RC_CHANNELS（0x1A，飞→站，默认关，1 Hz）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| time_boot_ms | u32 | ms |
| chancount | u8 | 有效通道数 |
| chan1_raw..chan18_raw | u16 ×18 | us，65535=无效 |
| rssi | u8 | 0..100，255=未知 |

### SERVO_OUTPUT_RAW（0x1B，飞→站，默认关，1–10 Hz）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| time_usec | u32 | ms |
| port | u8 | 输出端口 |
| servo1_raw..servo16_raw | u16 ×16 | PWM us |

### PARAM_SET（0x33，站→飞，事件）

| 字段 | 类型 | 说明 |
|---|---|---|
| param_id | char[16] | 参数名 |
| param_value | f32 | 目标值 |
| param_type | u8 | OU 类型枚举（整数/浮点/枚举/位图语义） |
| reserved | u8 ×2 | 保留，填 0 |

### PARAM_VALUE（0x1D，飞→站，事件）

| 字段 | 类型 | 说明 |
|---|---|---|
| param_id | char[16] | 参数名 |
| param_value | f32 | 飞控实际保存值 |
| param_type | u8 | 同 PARAM_SET |
| param_count | u16 | 参数总数 |
| param_index | u16 | 当前序号 |

写参数后飞控回传**实际保存值**（可能因取整/限幅与请求值不同）。

## 6. 实现范围

- **实现且默认发送**（13 帧）：HEARTBEAT、SYS_STATUS、COMMAND_ACK、POSE_NED、
  EKF_STATUS_REPORT、VFR_HUD、GLOBAL_POSITION_INT、WATER_DEPTH、DISTANCE_SENSOR、
  MANUAL_CONTROL、COMMAND、PARAM_SET、PARAM_VALUE。
- **实现但默认关闭、CMD_SET_STREAM 按需开启**（3 帧）：GPS_RAW_INT、RC_CHANNELS、
  SERVO_OUTPUT_RAW。
- **占号保留、不实现**（2 帧）：EXTENDED_SYS_STATE（0x11）、STATUSTEXT（0x12）。

落地顺序：批次 A（HEARTBEAT / COMMAND / COMMAND_ACK / MANUAL_CONTROL，命令闭环）
→ 批次 B（遥测 10 帧 + 3 个默认关回显帧）→ 批次 C（参数 2 帧）。
每批走 `schema/protocol.yaml` → codegen → golden_gen → 三端同步的既定流程。
