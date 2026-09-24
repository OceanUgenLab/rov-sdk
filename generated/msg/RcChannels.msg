# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 遥控通道回显，默认关闭，由 CMD_SET_STREAM 开启
uint8 FRAME_TYPE=0x1A

# Payload fields
uint32 time_boot_ms
uint8 chancount
uint16[18] chan_raw
uint8 rssi
