# AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

# EKF 健康/方差，1 Hz；方差均为 ×100 编码
uint8 FRAME_TYPE=0x14

# Payload fields
uint16 flags
uint8 velocity_variance
uint8 pos_horiz_variance
uint8 pos_vert_variance
uint8 compass_variance
uint8 terrain_alt_variance
