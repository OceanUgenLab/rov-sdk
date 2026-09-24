# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 心跳，双向 1 Hz；armed 判断 = state >= ARMED 且 < CRITICAL
uint8 FRAME_TYPE=0x50

# Payload fields
uint8 mode
uint8 system_type
uint8 fw_version
uint8 system_state
