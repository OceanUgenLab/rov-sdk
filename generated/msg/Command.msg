# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 命令帧；飞控以 CommandAck 应答，超时 0.5–1 s 重发
uint8 FRAME_TYPE=0x32

# Payload fields
uint16 command
uint32 param
