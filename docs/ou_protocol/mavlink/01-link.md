# 01 连接、身份和链路状态

## HEARTBEAT（双向，P0）

| 字段 | 类型 | 单位/取值 | OU 建议 |
|---|---|---|---|
| `custom_mode` | `uint32` | ArduSub 自定义模式号 | `mode` |
| `type` | `uint8` | `MAV_TYPE` | `system_type` |
| `autopilot` | `uint8` | `MAV_AUTOPILOT` | `autopilot_type` |
| `base_mode` | `uint8` | 解锁、手动、稳定等位 | `base_mode` |
| `system_status` | `uint8` | `MAV_STATE` | `system_state` |
| `mavlink_version` | `uint8` | 通常为 3 | `protocol_version` |

飞控和上位机各发送一帧，建议周期 1 Hz。飞控应在心跳中明确当前模式和 armed 状态。

## SYS_STATUS（飞控→上位机，P0）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `onboard_control_sensors_present` | `uint32` | 已安装传感器位图 |
| `onboard_control_sensors_enabled` | `uint32` | 已启用传感器位图 |
| `onboard_control_sensors_health` | `uint32` | 传感器健康位图 |
| `load` | `uint16` | 百分比×10 |
| `voltage_battery` | `uint16` | mV，65535 表示未知 |
| `current_battery` | `int16` | cA，-1 表示未知 |
| `battery_remaining` | `int8` | %，-1 表示未知 |
| `drop_rate_comm` | `uint16` | 丢包率×100 |
| `errors_comm` | `uint16` | 通信错误数 |
| `errors_count1..4` | `uint16` | 系统错误计数 |

## EXTENDED_SYS_STATE（飞控→上位机，P0）

| 字段 | 类型 | 说明 |
|---|---|---|
| `vtol_state` | `uint8` | VTOL 状态，Sub 通常不使用 |
| `landed_state` | `uint8` | 着陆/在水面/飞行状态；OU 应扩展 Sub 水面、水下枚举 |

## AUTOPILOT_VERSION（飞控→上位机，P1）

| 字段 | 类型 | 说明 |
|---|---|---|
| `capabilities` | `uint64` | 支持能力位 |
| `flight_sw_version` | `uint32` | 固件版本编码 |
| `middleware_sw_version` | `uint32` | 中间件版本 |
| `os_sw_version` | `uint32` | 操作系统版本 |
| `board_version` | `uint32` | 板卡版本 |
| `vendor_id` | `uint16` | 厂商 ID |
| `product_id` | `uint16` | 产品 ID |
| `uid` | `uint64` | 唯一设备 ID |
| `flight_custom_version` | `uint8[8]` | 自定义版本 |

## STATUSTEXT（飞控→上位机，P0）

| 字段 | 类型 | 说明 |
|---|---|---|
| `severity` | `uint8` | `MAV_SEVERITY` |
| `text` | `char[50]` | UTF-8/ASCII 文本 |
| `id` | `uint16` | 多段文本 ID |
| `chunk_seq` | `uint8` | 分段序号 |

## COMMAND_ACK（飞控→上位机，P0）

| 字段 | 类型 | 说明 |
|---|---|---|
| `command` | `uint16` | 被确认的 `MAV_CMD` |
| `result` | `uint8` | 接受、拒绝、失败等 |
| `progress` | `uint8` | 进度百分比 |
| `result_param2` | `int32` | 结果附加参数 |
| `target_system` | `uint8` | 目标系统 |
| `target_component` | `uint8` | 目标组件 |

