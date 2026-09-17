# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 水深/距底/水温（Sub 关键帧），1–10 Hz
uint8 FRAME_TYPE=0x19

# Payload fields
uint32 time_boot_ms
uint8 id
uint8 healthy
int32 lat
int32 lng
float32 altitude
float32 bottom_distance
float32 terrain_height
float32 temperature
