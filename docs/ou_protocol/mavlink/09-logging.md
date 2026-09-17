# 09 日志和调试

## LOG_REQUEST_LIST（上位机→飞控，P1）

| 字段 | 类型 | 说明 |
|---|---|---|
| `start` | `uint16` | 起始日志号 |
| `end` | `uint16` | 结束日志号 |
| `target_system` / `target_component` | `uint8` | 目标 |

## LOG_ENTRY（飞控→上位机，P1）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `id` | `uint16` | 日志号 |
| `num_logs` | `uint16` | 总日志数 |
| `last_log_num` | `uint16` | 最后日志号 |
| `time_utc` | `uint32` | Unix 秒 |
| `size` | `uint32` | 字节数 |

## LOG_REQUEST_DATA / LOG_DATA（双向，P1）

| 消息 | 字段 | 类型 | 说明 |
|---|---|---|---|
| `LOG_REQUEST_DATA` | `id`, `ofs`, `count` | `uint16`, `uint32`, `uint32` | 请求日志偏移和长度 |
| `LOG_DATA` | `id`, `ofs`, `count`, `data[90]` | `uint16`, `uint32`, `uint8`, `uint8[90]` | 日志数据块 |

OU 可使用更大的数据块，但必须保留日志号、偏移、块长度和总长度，支持断点续传。

## STATUSTEXT / NAMED_VALUE_FLOAT / DEBUG

| 消息 | 字段 | 说明 |
|---|---|---|
| `STATUSTEXT` | `severity`, `text`, `id`, `chunk_seq` | 实时告警和提示 |
| `NAMED_VALUE_FLOAT` | `time_boot_ms`, `name[10]`, `value` | 命名调试值 |
| `DEBUG` | `time_boot_ms`, `ind`, `value` | 数字调试通道 |

