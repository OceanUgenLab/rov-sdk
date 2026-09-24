# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 六轴速度/角速度指令，10–50 Hz；杆量 0 = 该轴交由飞控自稳；sequence 不递增或输入超时（约 500 ms）即 failsafe
uint8 FRAME_TYPE=0x30

# Payload fields
uint16 sequence
int16 x
int16 y
int16 z
int16 p
int16 r
int16 yaw
