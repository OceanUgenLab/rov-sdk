# 02 姿态、AHRS 和 EKF

## ATTITUDE（飞控→上位机，P0）

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `roll` / `pitch` / `yaw` | `float` | rad，yaw 范围 -π..π |
| `rollspeed` / `pitchspeed` / `yawspeed` | `float` | rad/s |

## ATTITUDE_QUATERNION（飞控→上位机，P1）

| 字段 | 类型 | 说明 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `q1..q4` | `float` | 四元数，w、x、y、z |
| `rollspeed` / `pitchspeed` / `yawspeed` | `float` | rad/s |

## AHRS（飞控→上位机，P1）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `roll` / `pitch` / `yaw` | `float` | rad |
| `altitude` | `float` | m |
| `lat` / `lng` | `int32` | 度×1e7 |

## AHRS2（飞控→上位机，P2）

| 字段 | 类型 | 单位 |
|---|---|---|
| `roll` / `pitch` / `yaw` | `float` | rad |
| `altitude` | `float` | m |
| `lat` / `lng` | `int32` | 度×1e7 |

## EKF_STATUS_REPORT（飞控→上位机，P0）

| 字段 | 类型 | 说明 |
|---|---|---|
| `flags` | `uint16` | EKF 健康和融合状态位 |
| `velocity_variance` | `uint8` | 速度方差×100 |
| `pos_horiz_variance` | `uint8` | 水平位置方差×100 |
| `pos_vert_variance` | `uint8` | 垂直位置方差×100 |
| `compass_variance` | `uint8` | 罗盘方差×100 |
| `terrain_alt_variance` | `uint8` | 地形高度方差×100 |

## VIBRATION（飞控→上位机，P1）

| 字段 | 类型 | 单位 |
|---|---|---|
| `time_usec` | `uint64` | us |
| `vibration_x/y/z` | `float` | m/s² |
| `clipping_0/1/2` | `uint32` | 三轴削波计数 |

## VFR_HUD（飞控→上位机，P0）

| 字段 | 类型 | 单位 |
|---|---|---|
| `airspeed` / `groundspeed` | `float` | m/s |
| `heading` | `int16` | 度 |
| `throttle` | `uint16` | % |
| `alt` | `float` | m |
| `climb` | `float` | m/s |

## NAV_CONTROLLER_OUTPUT（飞控→上位机，P1）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `nav_roll` / `nav_pitch` | `float` | rad |
| `nav_bearing` / `target_bearing` | `int16` | 度 |
| `wp_dist` | `uint16` | m |
| `alt_error` / `aspd_error` / `xtrack_error` | `float` | m、m/s、m |

