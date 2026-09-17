# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# 系统健康 + 电池 + 链路统计 + 遥测开关，1 Hz
uint8 FRAME_TYPE=0x10

# Payload fields
uint32 sensors_present
uint32 sensors_enabled
uint32 sensors_health
uint16 load
uint16 voltage_total
uint16 voltage_cell_max
uint16 voltage_cell_min
int16 current_battery
int8 battery_remaining
int32 current_consumed
int16 battery_temperature
uint32 battery_fault_bitmask
uint16 drop_rate_comm
uint16 errors_comm
uint16[4] errors_count
uint32 stream_mask
