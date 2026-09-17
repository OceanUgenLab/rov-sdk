# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 执行器 PWM 输出回显，默认关闭，由 CMD_SET_STREAM 开启
uint8 FRAME_TYPE=0x1B

# Payload fields
uint32 time_boot_ms
uint8 port
uint16[16] servo_raw
