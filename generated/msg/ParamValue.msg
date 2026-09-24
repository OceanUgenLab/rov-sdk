# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 参数值回读（飞控实际保存值，非请求值回显）
uint8 FRAME_TYPE=0x1D

# Payload fields
char[16] param_id
float32 param_value
uint8 param_type
uint16 param_count
uint16 param_index
