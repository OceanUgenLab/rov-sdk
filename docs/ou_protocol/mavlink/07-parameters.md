# 07 参数管理

## PARAM_REQUEST_LIST（上位机→飞控，P1）

| 字段 | 类型 | 说明 |
|---|---|---|
| `target_system` | `uint8` | 目标系统 |
| `target_component` | `uint8` | 目标组件 |

## PARAM_REQUEST_READ（上位机→飞控，P1）

| 字段 | 类型 | 说明 |
|---|---|---|
| `param_id` | `char[16]` | 参数名；与 `param_index` 二选一 |
| `param_index` | `int16` | 序号，-1 表示不用 |
| `target_system` / `target_component` | `uint8` | 目标 |

## PARAM_SET（上位机→飞控，P0）

| 字段 | 类型 | 说明 |
|---|---|---|
| `param_id` | `char[16]` | 参数名 |
| `param_value` | `float` | 参数值 |
| `param_type` | `uint8` | 参数类型 |
| `target_system` / `target_component` | `uint8` | 目标 |

## PARAM_VALUE（飞控→上位机，P0）

| 字段 | 类型 | 说明 |
|---|---|---|
| `param_id` | `char[16]` | 参数名 |
| `param_value` | `float` | 当前实际值 |
| `param_type` | `uint8` | 参数类型 |
| `param_count` | `uint16` | 总参数数 |
| `param_index` | `uint16` | 当前序号 |

OU 应用自己的 `param_type` 枚举，但必须保持整数、浮点、枚举和位图的语义。写参数后应回传飞控实际保存值，而不是简单回显请求值。

