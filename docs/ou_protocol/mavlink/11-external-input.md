# 11 外部传感器和视觉输入

## VISION_POSITION_ESTIMATE（上位机→飞控，P1）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `usec` | `uint64` | 时间戳 |
| `x` / `y` / `z` | `float` | m |
| `roll` / `pitch` / `yaw` | `float` | rad |
| `covariance` | `float[21]` | 上三角协方差；未知为 NaN |
| `reset_counter` | `uint8` | 估计器重置计数 |

## ODOMETRY（上位机→飞控，P1）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_usec` | `uint64` | 时间戳 |
| `frame_id` / `child_frame_id` | `uint8` | 坐标系 |
| `x` / `y` / `z` | `float` | m |
| `q` | `float[4]` | 姿态四元数 |
| `vx` / `vy` / `vz` | `float` | m/s |
| `rollspeed` / `pitchspeed` / `yawspeed` | `float` | rad/s |
| `pose_covariance` | `float[21]` | 位姿协方差 |
| `velocity_covariance` | `float[21]` | 速度协方差 |
| `reset_counter` | `uint8` | 重置计数 |
| `estimator_type` | `uint8` | 估计器类型 |
| `quality` | `int8` | 质量，-1 未知 |

## VISION_SPEED_ESTIMATE（上位机→飞控，P2）

| 字段 | 类型 | 单位 |
|---|---|---|
| `usec` | `uint64` | 时间戳 |
| `x` / `y` / `z` | `float` | m/s |
| `covariance` | `float[9]` | 速度协方差 |

## GPS_INPUT（上位机→飞控，P2）

| 字段 | 类型 | 单位/说明 |
|---|---|---|
| `time_usec` | `uint64` | 时间戳 |
| `gps_id` | `uint8` | GPS ID |
| `ignore_flags` | `uint16` | 忽略字段位图 |
| `time_week` / `time_week_ms` | `uint16` / `uint32` | GPS 时间 |
| `fix_type` | `uint8` | 定位类型 |
| `lat` / `lon` | `int32` | 度×1e7 |
| `alt` | `float` | m |
| `hdop` / `vdop` | `float` | 精度 |
| `vn` / `ve` / `vd` | `float` | m/s，NED |
| `speed_accuracy` / `horiz_accuracy` / `vert_accuracy` | `float` | m/s 或 m |
| `satellites_visible` | `uint8` | 颗数 |
| `yaw` | `uint16` | cdeg |

## 外部输入安全

视觉、里程计、GPS 和测距输入必须带源时间戳、质量、协方差/精度和重置计数。飞控必须在输入超时或质量无效时停止融合，而不是继续使用最后一帧数据。
