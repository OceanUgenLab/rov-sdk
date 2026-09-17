# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 命令应答，与 Command 的 command 字段对号
uint8 FRAME_TYPE=0x51

# Payload fields
uint16 command
uint8 result
uint8 progress
int32 result_param2
