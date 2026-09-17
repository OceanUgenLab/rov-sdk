# 03 位置、GPS、深度和导航

## GLOBAL_POSITION_INT（飞控→上位机，P0）

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `lat` / `lon` | `int32` | 度×1e7 |
| `alt` | `int32` | mm，AMSL |
| `relative_alt` | `int32` | mm，相对 Home |
| `vx` / `vy` / `vz` | `int16` | cm/s，NED |
| `hdg` | `uint16` | cdeg，65535 未知 |

## LOCAL_POSITION_NED（飞控→上位机，P0）

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `x` / `y` / `z` | `float` | m，NED |
| `vx` / `vy` / `vz` | `float` | m/s，NED |

## GPS_RAW_INT / GPS2_RAW（飞控→上位机，P0/P2）

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_usec` | `uint64` | us |
| `fix_type` | `uint8` | 定位类型 |
| `lat` / `lon` | `int32` | 度×1e7 |
| `alt` | `int32` | mm |
| `eph` / `epv` | `uint16` | cm×100，未知为 65535 |
| `vel` | `uint16` | cm/s |
| `cog` | `uint16` | cdeg |
| `satellites_visible` | `uint8` | 颗数 |
| `h_acc` / `v_acc` / `vel_acc` / `hdg_acc` | `uint32` | mm 或 cdeg×100 |

## HOME_POSITION（飞控→上位机，P1）

| 字段 | 类型 | 单位 |
|---|---|---|
| `latitude` / `longitude` | `int32` | 度×1e7 |
| `altitude` | `int32` | mm |
| `x` / `y` / `z` | `float` | m，本地 NED |
| `q` | `float[4]` | 四元数 |
| `approach_x/y/z` | `float` | m |

## WATER_DEPTH（飞控→上位机，P0）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `id` | `uint8` | 传感器 ID |
| `healthy` | `uint8` | 健康状态 |
| `lat` / `lng` | `int32` | 度×1e7，可选 |
| `altitude` | `float` | 传感器高度，m |
| `bottom_distance` | `float` | 到水底距离，m |
| `terrain_height` | `float` | 地形高度，m |
| `temperature` | `float` | °C |

## DISTANCE_SENSOR（双向，P0）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `min_distance` / `max_distance` | `uint16` | cm |
| `current_distance` | `uint16` | cm |
| `type` | `uint8` | 测距类型 |
| `id` | `uint8` | 传感器 ID |
| `orientation` | `uint8` | 方向 |
| `covariance` | `uint8` | cm²，未知为 255 |
| `horizontal_fov` / `vertical_fov` | `float` | rad |
| `signal_quality` | `uint8` | 0..100，未知为 255 |

## SCALED_PRESSURE / 2 / 3（飞控→上位机，P1/P2）

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `press_abs` | `float` | hPa |
| `press_diff` | `float` | hPa |
| `temperature` | `int16` | °C×100 |
| `temperature_press` | `int16` | 第二温度，°C×100 |

