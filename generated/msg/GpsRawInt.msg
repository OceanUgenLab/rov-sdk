# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# GPS 原始回显，默认关闭，由 CMD_SET_STREAM 开启
uint8 FRAME_TYPE=0x18

# Payload fields
uint64 time_usec
uint8 fix_type
int32 lat
int32 lon
int32 alt
uint16 eph
uint16 epv
uint16 vel
uint16 cog
uint8 satellites_visible
uint32 h_acc
uint32 v_acc
uint32 vel_acc
uint32 hdg_acc
