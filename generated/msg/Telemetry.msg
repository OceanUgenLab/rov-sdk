# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# Mode 枚举
uint8 MODE_MANUAL=0
uint8 MODE_AUTO=1
uint8 MODE_RETURN=2
uint8 MODE_HOLD=3

# Payload fields
float32 roll
float32 pitch
float32 heading
float32 yaw_rate
float32 depth
float32 altitude
float32 north
float32 east
float32 vn
float32 ve
float32 vd
float32 voltage
float32 current
float32 percent
float32 temperature
float32 water_temp
float32 salinity
float32 pressure_bar
float32 cable_tension
float32[8] thr
uint8 leak
uint8 armed
uint8 mode
uint8 reserved
uint8[16] reserved_tail
