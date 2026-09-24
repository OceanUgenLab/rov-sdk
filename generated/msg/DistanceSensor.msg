# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 测距（避碰声呐），事件/1 Hz
uint8 FRAME_TYPE=0x52

# Payload fields
uint32 time_boot_ms
uint16 min_distance
uint16 max_distance
uint16 current_distance
uint8 type
uint8 id
uint8 orientation
uint8 covariance
float32 horizontal_fov
float32 vertical_fov
uint8 signal_quality
