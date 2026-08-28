# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# Mode 枚举
uint8 MODE_MANUAL=0
uint8 MODE_AUTO=1
uint8 MODE_RETURN=2
uint8 MODE_HOLD=3

# Payload fields
uint8 mode
uint8 armed
uint8[2] reserved
float32 surge
float32 sway
float32 heave
float32 yaw
float32 target_depth
float32 target_heading
float32 target_north
float32 target_east
uint8[16] reserved_tail
