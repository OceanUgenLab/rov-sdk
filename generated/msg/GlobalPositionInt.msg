# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 全局经纬高 + NED 速度，1 Hz
uint8 FRAME_TYPE=0x16

# Payload fields
uint32 time_boot_ms
int32 lat
int32 lon
int32 alt
int32 relative_alt
int16 vx
int16 vy
int16 vz
uint16 hdg
