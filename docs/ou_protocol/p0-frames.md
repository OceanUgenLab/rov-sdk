# OU 协议 P0 帧整理与格式定义

> 来源：`docs/ou_protocol/mavlink/01..11-*.md` 中标记为 **P0** 的帧。
> 本文档是 P0 需求的**整理稿与格式建议**，不是协议真源。落地时按工作流程写入
> `schema/protocol.yaml`，由 `tools/codegen.py` 生成正式文档，禁止直接实现本文。

## P0 帧清单与 type 分配（共 18 帧）

type 号段规则：`0x01` CmdPacket、`0x02` TelemetryPacket **均废弃**（批次 A 落地时从 schema 一并删除，三端直接切换到 P0 帧，两号不回收）；`0x10..0x2F` 上行（飞控→上位机）；`0x30..0x4F` 下行（上位机→飞控）；`0x50..0x5F` 双向。

| # | 帧 | type | 方向 | 源文档 | 用途 |
|---|----|------|------|--------|------|
| 1 | HEARTBEAT | `0x50` | 双向 | 01 | 心跳、模式与状态，1 Hz |
| 2 | SYS_STATUS | `0x10` | 飞→上 | 01/06 | 系统健康 + 电池状态（源 BATTERY_STATUS 并入）、负载、丢包率 |
| 3 | EXTENDED_SYS_STATE | `0x11` | 飞→上 | 01 | 载体水面/水下状态；**P0 保留但不实现** |
| 4 | COMMAND_ACK | `0x51` | 飞→上 | 01 | 命令应答 |
| 5 | STATUSTEXT | `0x12` | 飞→上 | 01 | 文本告警；**P0 保留但不实现** |
| 6 | POSE_NED | `0x13` | 飞→上 | 02/03 | 姿态 + 本地 NED 位置/速度（源 ATTITUDE 与 LOCAL_POSITION_NED 合并） |
| 7 | EKF_STATUS_REPORT | `0x14` | 飞→上 | 02 | EKF 健康/方差 |
| 8 | VFR_HUD | `0x15` | 飞→上 | 02 | 速度、航向、油门、高度、爬升率 |
| 9 | GLOBAL_POSITION_INT | `0x16` | 飞→上 | 03 | 全局位置（经纬高 + NED 速度） |
| 10 | GPS_RAW_INT | `0x18` | 飞→上 | 03 | GPS 定位原始数据；**默认关闭**，CMD_SET_STREAM 开启 |
| 11 | WATER_DEPTH | `0x19` | 飞→上 | 03 | 水深/高度/水温（Sub 关键帧） |
| 12 | DISTANCE_SENSOR | `0x52` | 双向 | 03 | 测距（避碰声呐等） |
| 13 | MANUAL_CONTROL | `0x30` | 上→飞 | 04 | 手动操控轴 |
| 14 | COMMAND | `0x32` | 上→飞 | 04/01 | 命令帧（SET_MODE / ARM / DISARM / GO_HOME…，源 SET_MODE 与 COMMAND_LONG 合并） |
| 15 | RC_CHANNELS | `0x1A` | 飞→上 | 05 | 遥控器通道回显；**默认关闭**，CMD_SET_STREAM 开启 |
| 16 | SERVO_OUTPUT_RAW | `0x1B` | 飞→上 | 05 | 舵机/电机 PWM 输出；**默认关闭**，CMD_SET_STREAM 开启 |
| 17 | PARAM_SET | `0x33` | 上→飞 | 07 | 写参数 |
| 18 | PARAM_VALUE | `0x1D` | 飞→上 | 07 | 参数值（读回/回显实际保存值） |

## 帧封套（与现行 v0.2.0 一致）

所有 P0 帧复用 `schema/protocol.yaml` 定义的帧级格式，不另设封套：

```
| STX (0xAA 0x55) | ver (u8, 0x02) | len (u8, payload 字节数) | type (u8) | payload | crc16 (le, CRC-16/MODBUS) |
```

- 字节序固定小端；浮点一律 IEEE-754 `float32`。
- CRC 覆盖 `ver` 起共 `3 + len` 字节，不含 STX，不含 CRC 自身。
- type 分配见上方清单表：`0x01` CmdPacket 与 `0x02` TelemetryPacket 批次 A 落地时一并废弃删除（破坏性变更，schema 版本号提升，两号不回收，不设过渡期——三端同版本一次切换）；上行 P0 帧 `0x10..0x1D`（余量至 0x2F），下行 P0 帧 `0x30..0x33`（余量至 0x4F），双向 P0 帧 `0x50..0x52`（余量至 0x5F）。

## 载荷格式定义

下表字段类型沿用 schema 别名（`u8/u16/u32/i8/i16/i32/f32`）。除特别注明外，
字段语义、单位与 MAVLink 源定义一致；「OU 调整」列记录对源协议的改动要求。

### 1. HEARTBEAT（type=0x50，双向，周期 1 Hz）

| 字段 | 类型 | 单位/取值 | OU 调整 |
|---|---|---|---|
| `mode` | u8 | OU Mode 枚举：0=MANUAL, 1=AUTO, 2=RETURN, 3=HOLD | 不用源 `custom_mode`(u32)，直接复用 schema 既有 Mode 枚举 |
| `system_type` | u8 | bit0=角色：0=机器人，1=地面站；bit1..7=产品序号 | 不用 MAV_TYPE，见下方定义 |
| `fw_version` | u8 | 固件版本（主版本号 0..255） | 不用 MAV_AUTOPILOT，见下方定义 |
| `system_state` | u8 | OU 状态枚举，见下方定义 | 不用 MAV_STATE |

**OU 调整**：去掉源 `base_mode` 位图，**armed 状态并入 `system_state`**，工作模式只看 `mode`，职责单一不冗余。

**OU `system_state` 枚举定义**（u8）：

| 值 | 名称 | 含义 |
|---|------|------|
| 0 | UNINIT | 未初始化，上电自检中 |
| 1 | BOOT | 启动中（引导/系统初始化阶段） |
| 2 | CALIBRATING | 校准中，不可解锁 |
| 3 | STANDBY | 待命，就绪但未解锁 |
| 4 | ARMED | 已解锁，推进器可出力 |
| 5 | ACTIVE | 作业中（已解锁且正在执行任务） |
| 6 | CRITICAL | 严重故障，可挽救（立即上浮/返航） |
| 7 | EMERGENCY | 紧急，无法挽救（漏水/失控），执行安全行为 |
| 8..255 | 保留 | 接收方忽略未知值 |

相对 MAV_STATE：去掉 POWER_OFF（断电发不出心跳）、FLIGHT_TERMINATION；新增 ARMED 使 armed 判断无需第二个字段。

**OU `system_type` 编码定义**（u8，位域）：

| 位 | 含义 | 取值 |
|---|------|------|
| bit0 | 角色 | 0 = 机器人（飞控/载具侧），1 = 地面站（上位机侧） |
| bit1..7 | 产品序号 | 当前定义两种机型：0、1；其余值保留，接收方应忽略未知机型 |

示例：机器人侧机型 0 → `0x00`，机器人侧机型 1 → `0x02`，地面站 → `0x01`（地面站不带机型，产品序号填 0）。

**OU `autopilot_type` 字段重定义为 `fw_version`（固件主版本号，u8）**：

| 字段 | 类型 | 说明 |
|---|---|---|
| `fw_version` | u8 | 飞控固件主版本号，0..255；地面站侧心跳填自身 SDK 主版本号 |

仅存主版本号，次要版本/修订号如需完整查询走 `AUTOPILOT_VERSION`（P1，`flight_sw_version` 含完整 maj.min.patch.rev 编码）。上位机据此做固件代际分支（如 v1 忽略 `target_north/east`，v2 支持）。

### 2. SYS_STATUS（type=0x10，飞→上）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `sensors_present` | u32 | 已安装传感器位图 |
| `sensors_enabled` | u32 | 已启用传感器位图 |
| `sensors_health` | u32 | 传感器健康位图 |
| `load` | u16 | CPU 负载 %×10 |
| `voltage_total` | u16 | 电池总电压 mV，65535=未知 |
| `voltage_cell_max` / `voltage_cell_min` | u16 ×2 | 单体最高/最低电压 mV，65535=未知 |
| `current_battery` | i16 | cA，-1=未知 |
| `battery_remaining` | i8 | %，-1=未知 |
| `current_consumed` | i32 | mAh，-1=未知 |
| `battery_temperature` | i16 | °C×100 |
| `battery_fault_bitmask` | u32 | 电池故障位图 |
| `drop_rate_comm` | u16 | 丢包率 %×100 |
| `errors_comm` | u16 | 通信错误计数 |
| `errors_count1..4` | u16 ×4 | 系统错误计数 |
| `stream_mask` | u32 | 遥测开关位图，见下方定义 |

**`stream_mask` 位定义**（CMD_SET_STREAM 控制的回显帧开关状态，1=发送中）：

| 位 | 对应帧 |
|---|------|
| bit0 | GPS_RAW_INT (`0x18`) |
| bit1 | RC_CHANNELS (`0x1A`) |
| bit2 | SERVO_OUTPUT_RAW (`0x1B`) |
| bit3..31 | 保留，置 0 |

上位机发 `CMD_SET_STREAM` 收到 ACK 后，可从本字段确认开关实际生效状态
（含飞控重启后回到默认全关的情景）。

**OU 调整**：源 `BATTERY_STATUS`(0x1C) 并入本帧（首版单电池，去 `id` 字段），
`0x1C` 释放为保留号。电池与系统健康均为 1 Hz 级低频遥测，合并省一帧开销。
将来多电池或电池细节需求（时间剩余、充电状态）再启用独立帧。

### 3. EXTENDED_SYS_STATE（type=0x11，飞→上）——保留但不实现

P0 阶段**不实现**：其载体运动状态（水面/水下/坐底）可由深度与垂向速度在接收端推断，且与 HEARTBEAT `system_state` 有重叠，待实际需要时再落地。type `0x11` 为其保留占位，其他帧不得占用。

| 字段 | 类型 | OU 调整 |
|---|---|---|
| `vtol_state` | u8 | Sub 不使用，保留占位 |
| `landed_state` | u8 | **OU 需扩展枚举**：水面、水下、坐底等 Sub 专属状态 |

### 4. COMMAND_ACK（type=0x51，飞→上）

| 字段 | 类型 | 说明 |
|---|---|---|
| `command` | u16 | 被确认的命令号 |
| `result` | u8 | 接受/拒绝/失败枚举 |
| `progress` | u8 | 进度 % |
| `result_param2` | i32 | 附加结果参数 |
| `target_system` | u8 | |
| `target_component` | u8 | |

### 5. STATUSTEXT（type=0x12，飞→上）——保留但不实现

P0 阶段**不实现**：文本告警非关键路径，事件信息先由结构化帧承载（SYS_STATUS 位图、COMMAND_ACK result）；现场排障如有需要再启用。type `0x12` 为其保留占位，其他帧不得占用。

| 字段 | 类型 | 说明 |
|---|---|---|
| `severity` | u8 | 严重级枚举（MAV_SEVERITY 对应，OU 可精简） |
| `text` | char[50] | UTF-8/ASCII 正文 |
| `id` | u16 | 多段文本 ID，同一长消息各分段相同 |
| `chunk_seq` | u8 | 分段序号，从 0 递增 |

### 6. POSE_NED（type=0x13，飞→上）——ATTITUDE 与 LOCAL_POSITION_NED 合并

姿态与本地位置/速度同源同频（均出自 AHRS/EKF，姿态闭环和位置闭环通常一起消费），
OU 合并为一帧，节省一次帧头/CRC 开销并保证姿态与位置时间戳严格一致。

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_boot_ms` | u32 | ms |
| `roll` / `pitch` / `yaw` | f32 ×3 | rad，yaw ∈ -π..π |
| `rollspeed` / `pitchspeed` / `yawspeed` | f32 ×3 | rad/s |
| `x` / `y` / `z` | f32 ×3 | m，NED |
| `vx` / `vy` / `vz` | f32 ×3 | m/s，NED |

**OU 调整**：源 `ATTITUDE`(0x13) 与 `LOCAL_POSITION_NED`(0x17) 合并为本帧，占 `0x13`；
`0x17` 释放为保留号，其他帧不得占用。载荷 52 字节（4 + 12×f32），高频下发（建议 10–50 Hz）。

### 7. EKF_STATUS_REPORT（type=0x14，飞→上）

| 字段 | 类型 | 说明 |
|---|---|---|
| `flags` | u16 | EKF 健康/融合状态位 |
| `velocity_variance` | u8 | ×100 |
| `pos_horiz_variance` | u8 | ×100 |
| `pos_vert_variance` | u8 | ×100 |
| `compass_variance` | u8 | ×100 |
| `terrain_alt_variance` | u8 | ×100 |

### 8. VFR_HUD（type=0x15，飞→上）

| 字段 | 类型 | 单位 |
|---|---|---|
| `airspeed` / `groundspeed` | f32 ×2 | m/s |
| `heading` | i16 | deg |
| `throttle` | u16 | % |
| `alt` | f32 | m |
| `climb` | f32 | m/s |

### 9. GLOBAL_POSITION_INT（type=0x16，飞→上）

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_boot_ms` | u32 | ms |
| `lat` / `lon` | i32 ×2 | deg×1e7 |
| `alt` | i32 | mm，AMSL |
| `relative_alt` | i32 | mm，相对 Home |
| `vx` / `vy` / `vz` | i16 ×3 | cm/s，NED |
| `hdg` | u16 | cdeg，65535=未知 |

### 10. GPS_RAW_INT（type=0x18，飞→上）——**默认关闭**，CMD_SET_STREAM 开启

GPS 原始回显，调试/记录用；**默认不发送**（水下无 GPS 时无意义），
由 `CMD_SET_STREAM(type=0x18, on/off)` 控制开关。

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_usec` | u64 | us |
| `fix_type` | u8 | 定位类型枚举 |
| `lat` / `lon` | i32 ×2 | deg×1e7 |
| `alt` | i32 | mm |
| `eph` / `epv` | u16 ×2 | cm×100，65535=未知 |
| `vel` | u16 | cm/s |
| `cog` | u16 | cdeg |
| `satellites_visible` | u8 | 颗 |
| `h_acc` / `v_acc` / `vel_acc` / `hdg_acc` | u32 ×4 | mm / cdeg×100 |

### 11. WATER_DEPTH（type=0x19，飞→上，Sub 关键帧）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_boot_ms` | u32 | ms |
| `id` | u8 | 传感器 ID |
| `healthy` | u8 | 健康状态 |
| `lat` / `lng` | i32 ×2 | deg×1e7，可选 |
| `altitude` | f32 | 传感器高度 m |
| `bottom_distance` | f32 | 到水底距离 m |
| `terrain_height` | f32 | 地形高度 m |
| `temperature` | f32 | °C |

### 12. DISTANCE_SENSOR（type=0x52，双向）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_boot_ms` | u32 | ms |
| `min_distance` / `max_distance` | u16 ×2 | cm |
| `current_distance` | u16 | cm |
| `type` | u8 | 测距类型 |
| `id` | u8 | 传感器 ID |
| `orientation` | u8 | 朝向 |
| `covariance` | u8 | cm²，255=未知 |
| `horizontal_fov` / `vertical_fov` | f32 ×2 | rad |
| `signal_quality` | u8 | 0..100，255=未知 |

### 13. MANUAL_CONTROL（type=0x30，上→飞）

| 字段 | 类型 | 单位/取值 |
|---|---|---|
| `sequence` | u16 | 递增序号，丢包/乱序检测 |
| `x` | i16 | 前后速度（surge）-1000..1000 |
| `y` | i16 | 横移速度（sway）-1000..1000 |
| `z` | i16 | 升沉速度（heave）-1000..1000 |
| `p` | i16 | 俯仰角速度（pitch rate）-1000..1000 |
| `r` | i16 | 横滚角速度（roll rate）-1000..1000 |
| `yaw` | i16 | 偏航角速度（yaw rate）-1000..1000 |

**OU 调整**：六轴速度/角速度指令（x/y/z + p/r/yaw），载荷 14 字节。源 `buttons`、
扩展轴（`s`/`t`/`aux1..6`、`enabled_extensions`）、`timestamp_ms`、`valid_mask`
删除；`sequence` 保留用于丢包检测。六轴均为速度指令，**填 0 = 该轴交由飞控自稳**
（增稳/自动模式下闭环保持定深、定向等），不存在"通道缺失"问题——不需要 valid_mask。
失控保护：sequence 不递增或输入超时（建议 500 ms）即 failsafe。

### 14. COMMAND（type=0x32，上→飞）——含模式切换的命令帧

代替源 `COMMAND_LONG`（7 匿名浮点）与 `SET_MODE`：OU 只设一条**命令帧**，
`command` 为 OU 自有命令枚举，`param` 按命令定义解释；模式切换（原 SET_MODE）
与解锁/上锁均作为命令承载，飞控统一用 COMMAND_ACK 应答。

| 字段 | 类型 | 说明 |
|---|---|---|
| `command` | u16 | OU Command 枚举，见下表 |
| `param` | u32 | 命令参数，按命令定义解释（枚举传整数值） |

**OU Command 枚举（首版）**：

| 值 | 命令 | param 含义 |
|---|------|-----------|
| 0x0001 | CMD_SET_MODE | Mode 枚举值（同 HEARTBEAT 的 `mode`） |
| 0x0002 | CMD_ARM | 低 16 位：0=上锁（DISARM），1=解锁（ARM）；高 16 位=0 |
| 0x0003 | 保留 | （原 CMD_DISARM，并入 CMD_ARM param=0） |
| 0x0004 | CMD_GO_HOME | 0（预留） |
| 0x0005 | CMD_SET_STREAM | 遥测开关：高 16 位=目标帧 type，低 16 位=0 关 / 1 开 |
| 0x0006 | CMD_SET_PWM | 直控输出：高 16 位=通道号 1..16，低 16 位=PWM us（1100..1900） |

`CMD_SET_STREAM` 用于按需开启调试/原始回显帧（GPS_RAW_INT、RC_CHANNELS、
SERVO_OUTPUT_RAW），**三帧默认关闭**，需要时逐帧打开，不需要时关闭省带宽；
开关状态仅存于飞控运行内存，重启后恢复默认关闭。

`CMD_SET_PWM` 直接设置单路舵机/备用输出通道 PWM（云台、灯光、机械臂等任务载荷）：
不参与推进器混控，仅作用于混控之外的辅助输出通道；飞控应校验通道号与 PWM 范围，
非法值回 COMMAND_ACK(DENIED)。可配合 SERVO_OUTPUT_RAW（CMD_SET_STREAM 开启）
观察实际输出验证效果。

**OU 调整**：

- 不用源 COMMAND_LONG 的 7 个匿名浮点，只留一个 `param`（u32）——参数超过一个的命令
  应定义具名字段的专用帧，而非扩匿名参数；
- SET_MODE 并入为 `CMD_SET_MODE`，源帧废弃，type `0x31` 释放为保留号；
- 去掉 `target_*`（点对点无需寻址）与 `confirmation`（重发由上位机超时重发实现）。

交互闭环：上位机发 COMMAND → 飞控回 COMMAND_ACK（`command` 对号）；
模式切换成功后下一帧 HEARTBEAT 的 `mode` 可确认生效。

### 15. RC_CHANNELS（type=0x1A，飞→上）——**默认关闭**，CMD_SET_STREAM 开启

实现但**默认不发送**：操控输入均经上位机走 MANUAL_CONTROL 下发，本帧仅作
遥控直连场景/通道调试的原始回显，由 `CMD_SET_STREAM(type=0x1A, on/off)` 控制开关。

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_boot_ms` | u32 | ms |
| `chancount` | u8 | 有效通道数 |
| `chan1_raw..chan18_raw` | u16 ×18 | us，65535=无效 |
| `rssi` | u8 | 0..100，255=未知 |

### 16. SERVO_OUTPUT_RAW（type=0x1B，飞→上）——**默认关闭**，CMD_SET_STREAM 开启

执行器输出回显，混控调试/故障诊断用；带宽较大（载荷 37 字节），**默认不发送**，
由 `CMD_SET_STREAM(type=0x1B, on/off)` 控制开关。

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_usec` | u32 | ms 级时间戳（源 u32/u64 两种，OU 定为 u32 ms） |
| `port` | u8 | 输出端口 |
| `servo1_raw..servo16_raw` | u16 ×16 | PWM us |

### 17. PARAM_SET（type=0x33，上→飞）

| 字段 | 类型 | 说明 |
|---|---|---|
| `param_id` | char[16] | 参数名 |
| `param_value` | f32 | 目标值 |
| `param_type` | u8 | OU 自有类型枚举（保持整数/浮点/枚举/位图语义） |

（已去掉 `target_*` 寻址字段）

### 18. PARAM_VALUE（type=0x1D，飞→上）

| 字段 | 类型 | 说明 |
|---|---|---|
| `param_id` | char[16] | 参数名 |
| `param_value` | f32 | **飞控实际保存值**，不是请求值回显 |
| `param_type` | u8 | 同 PARAM_SET |
| `param_count` | u16 | 参数总数 |
| `param_index` | u16 | 当前序号 |

## P0 落地批次建议

按依赖关系分三批进入 `schema/protocol.yaml`：

1. **批次 A：链路保活与命令闭环**（先行，其余帧依赖它）
   HEARTBEAT、COMMAND、COMMAND_ACK、MANUAL_CONTROL。
   COMMAND 首版承载 CMD_SET_MODE / CMD_ARM(含上锁) / CMD_GO_HOME / CMD_SET_STREAM / CMD_SET_PWM。
2. **批次 B：状态遥测**
   SYS_STATUS、POSE_NED、EKF_STATUS_REPORT、VFR_HUD、
   GLOBAL_POSITION_INT、WATER_DEPTH、DISTANCE_SENSOR。
   其中 GPS_RAW_INT / RC_CHANNELS / SERVO_OUTPUT_RAW 三帧同样实现但**默认关闭**，
   由 CMD_SET_STREAM 按需开启（依赖批次 A 的 COMMAND）。
3. **批次 C：参数管理**
   PARAM_SET、PARAM_VALUE。

每批落地流程：`schema/protocol.yaml` 增帧 → `tools/codegen.py` → `tools/golden_gen.py`
→ golden 与测试红绿循环 → 三端同步。
