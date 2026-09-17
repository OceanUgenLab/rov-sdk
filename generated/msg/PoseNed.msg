# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 姿态 + 本地 NED 位置/速度，10–50 Hz；姿态与位置共享时间戳
uint8 FRAME_TYPE=0x13

# Payload fields
uint32 time_boot_ms
float32 roll
float32 pitch
float32 yaw
float32 rollspeed
float32 pitchspeed
float32 yawspeed
float32 x
float32 y
float32 z
float32 vx
float32 vy
float32 vz
