# 10 相机、云台和外设

## CAMERA_INFORMATION（飞控→上位机，P2）

| 字段 | 类型 | 说明 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `vendor_name` / `model_name` | `char[32]` | 厂商和型号 |
| `firmware_version` | `uint32` | 版本 |
| `focal_length` | `float` | mm |
| `sensor_size_h` / `sensor_size_v` | `float` | mm |
| `resolution_h` / `resolution_v` | `uint16` | 像素 |
| `lens_id` | `uint8` | 镜头 ID |
| `flags` | `uint32` | 相机能力位 |

## CAMERA_SETTINGS（双向，P2）

| 字段 | 类型 | 说明 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `mode_id` | `uint8` | 相机模式 |
| `zoomLevel` | `float` | 变焦级别 |
| `focusLevel` | `float` | 对焦级别 |

## CAMERA_CAPTURE_STATUS（飞控→上位机，P2）

| 字段 | 类型 | 说明 |
|---|---|---|
| `time_boot_ms` | `uint32` | ms |
| `image_status` | `uint8` | 拍摄状态 |
| `video_status` | `uint8` | 录像状态 |
| `image_interval` | `float` | s |
| `recording_time_ms` | `uint32` | ms |
| `available_capacity` | `float` | MB |

## GIMBAL_DEVICE_ATTITUDE_STATUS（飞控→上位机，P2）

| 字段 | 类型 | 说明 |
|---|---|---|
| `time_boot_us` | `uint64` | us |
| `flags` | `uint16` | 云台状态位 |
| `q` | `float[4]` | 云台姿态四元数 |
| `angular_velocity_x/y/z` | `float` | rad/s |
| `failure_flags` | `uint32` | 故障位 |
| `device_id` | `uint8` | 设备 ID |

## GIMBAL_MANAGER_SET_ATTITUDE（上位机→飞控，P2）

| 字段 | 类型 | 说明 |
|---|---|---|
| `flags` | `uint32` | 控制字段有效位 |
| `gimbal_device_id` | `uint8` | 设备 ID |
| `q` | `float[4]` | 目标姿态 |
| `angular_velocity_x/y/z` | `float` | rad/s |

