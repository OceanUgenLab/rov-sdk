# 08 任务和航点

## MISSION_REQUEST_LIST / MISSION_COUNT（双向，P1）

| 消息 | 字段 | 类型 | 说明 |
|---|---|---|---|
| `MISSION_REQUEST_LIST` | `target_system`, `target_component`, `mission_type` | `uint8`, `uint8`, `uint8` | 请求任务数量 |
| `MISSION_COUNT` | `count`, `target_system`, `target_component`, `mission_type` | `uint16`, `uint8`, `uint8`, `uint8` | 任务数量和类型 |

## MISSION_REQUEST_INT（飞控→上位机，P1）

| 字段 | 类型 | 说明 |
|---|---|---|
| `seq` | `uint16` | 请求序号 |
| `target_system` / `target_component` | `uint8` | 目标 |
| `mission_type` | `uint8` | 任务类型 |

## MISSION_ITEM_INT（双向，P1）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `param1..param4` | `float` | 航点参数 |
| `x` / `y` | `int32` | 纬度/经度×1e7 |
| `z` | `float` | 高度/深度，m |
| `seq` | `uint16` | 序号 |
| `command` | `uint16` | `MAV_CMD` |
| `target_system` / `target_component` | `uint8` | 目标 |
| `frame` | `uint8` | 坐标系 |
| `current` | `uint8` | 是否当前任务 |
| `autocontinue` | `uint8` | 是否自动继续 |
| `mission_type` | `uint8` | 任务类型 |

## MISSION_ACK / MISSION_CURRENT / MISSION_ITEM_REACHED

| 消息 | 字段 | 说明 |
|---|---|---|
| `MISSION_ACK` | `type`, `mission_type` | 上传/下载结果和任务类型 |
| `MISSION_CURRENT` | `seq`, `total`, `mission_state`, `mission_mode` | 当前序号、总数、执行状态 |
| `MISSION_ITEM_REACHED` | `seq` | 到达的航点序号 |

任务传输必须使用超时、重试和序号校验，不能只依赖串口发送成功。

