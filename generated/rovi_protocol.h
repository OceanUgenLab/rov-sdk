/* AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml */
#ifndef ROVI_GENERATED_PROTOCOL_H
#define ROVI_GENERATED_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 帧常量 */
#define ROVI_STX0 0xAA
#define ROVI_STX1 0x55
#define ROVI_PROTOCOL_VERSION 0x02
#define ROVI_TYPE_CMD 0x01
#define ROVI_TYPE_TELE 0x02
#define ROVI_NUM_THRUSTERS 8
#define ROVI_FRAME_OVERHEAD 7
#define ROVI_CMD_PAYLOAD_SIZE 52
#define ROVI_TELE_PAYLOAD_SIZE 128

/* 字段偏移宏 */
#define ROVI_CMDPACKET_MODE_OFFSET 0
#define ROVI_CMDPACKET_ARMED_OFFSET 1
#define ROVI_CMDPACKET_RESERVED_OFFSET 2
#define ROVI_CMDPACKET_SURGE_OFFSET 4
#define ROVI_CMDPACKET_SWAY_OFFSET 8
#define ROVI_CMDPACKET_HEAVE_OFFSET 12
#define ROVI_CMDPACKET_YAW_OFFSET 16
#define ROVI_CMDPACKET_TARGET_DEPTH_OFFSET 20
#define ROVI_CMDPACKET_TARGET_HEADING_OFFSET 24
#define ROVI_CMDPACKET_TARGET_NORTH_OFFSET 28
#define ROVI_CMDPACKET_TARGET_EAST_OFFSET 32
#define ROVI_CMDPACKET_RESERVED_TAIL_OFFSET 36
#define ROVI_TELEMETRYPACKET_ROLL_OFFSET 0
#define ROVI_TELEMETRYPACKET_PITCH_OFFSET 4
#define ROVI_TELEMETRYPACKET_HEADING_OFFSET 8
#define ROVI_TELEMETRYPACKET_YAW_RATE_OFFSET 12
#define ROVI_TELEMETRYPACKET_DEPTH_OFFSET 16
#define ROVI_TELEMETRYPACKET_ALTITUDE_OFFSET 20
#define ROVI_TELEMETRYPACKET_NORTH_OFFSET 24
#define ROVI_TELEMETRYPACKET_EAST_OFFSET 28
#define ROVI_TELEMETRYPACKET_VN_OFFSET 32
#define ROVI_TELEMETRYPACKET_VE_OFFSET 36
#define ROVI_TELEMETRYPACKET_VD_OFFSET 40
#define ROVI_TELEMETRYPACKET_VOLTAGE_OFFSET 44
#define ROVI_TELEMETRYPACKET_CURRENT_OFFSET 48
#define ROVI_TELEMETRYPACKET_PERCENT_OFFSET 52
#define ROVI_TELEMETRYPACKET_TEMPERATURE_OFFSET 56
#define ROVI_TELEMETRYPACKET_WATER_TEMP_OFFSET 60
#define ROVI_TELEMETRYPACKET_SALINITY_OFFSET 64
#define ROVI_TELEMETRYPACKET_PRESSURE_BAR_OFFSET 68
#define ROVI_TELEMETRYPACKET_CABLE_TENSION_OFFSET 72
#define ROVI_TELEMETRYPACKET_THR_OFFSET 76
#define ROVI_TELEMETRYPACKET_LEAK_OFFSET 108
#define ROVI_TELEMETRYPACKET_ARMED_OFFSET 109
#define ROVI_TELEMETRYPACKET_MODE_OFFSET 110
#define ROVI_TELEMETRYPACKET_RESERVED_OFFSET 111
#define ROVI_TELEMETRYPACKET_RESERVED_TAIL_OFFSET 112

/* 控制指令载荷（主控 -> 从控） */
typedef struct __attribute__((packed)) {
    uint8_t mode; /* 运行模式 */
    uint8_t armed; /* 解锁标志，1=armed 输出推进器，0=停机 */
    uint8_t reserved[2]; /* 保留，发送方清零，接收方忽略 */
    float surge; /* 前(+) / 后(-) */
    float sway; /* 右平移(+) / 左平移(-) */
    float heave; /* 上浮(+) / 下潜(-) */
    float yaw; /* 右转(+) / 左转(-) */
    float target_depth; /* 目标深度 */
    float target_heading; /* 目标航向 */
    float target_north; /* v2, firmware v1 ignores（目标北向位移，寻迹/返航用） */
    float target_east; /* v2, firmware v1 ignores（目标东向位移，寻迹/返航用） */
    uint8_t reserved_tail[16]; /* 扩展预留区（云台/机械臂/照明等后置），发送方清零，接收方忽略 */
} CmdPacket;

/* 遥测载荷（从控 -> 主控） */
typedef struct __attribute__((packed)) {
    float roll; /* 横滚角 */
    float pitch; /* 俯仰角 */
    float heading; /* 航向角 */
    float yaw_rate; /* 偏航角速度 */
    float depth; /* 水深 */
    float altitude; /* 距底高度 */
    float north; /* 相对原点北向位移 */
    float east; /* 相对原点东向位移 */
    float vn; /* 北向速度 */
    float ve; /* 东向速度 */
    float vd; /* 下潜速度（向下为正） */
    float voltage; /* 电池电压 */
    float current; /* 电池电流 */
    float percent; /* 电池电量百分比 */
    float temperature; /* 电池/舱内温度 */
    float water_temp; /* 水温 */
    float salinity; /* 盐度 */
    float pressure_bar; /* 水压 */
    float cable_tension; /* 缆绳张力 */
    float thr[8]; /* 8 路推进器输出百分比 */
    uint8_t leak; /* 漏水检测，1=报警 */
    uint8_t armed; /* 当前 armed 状态 */
    uint8_t mode; /* 当前运行模式 */
    uint8_t reserved; /* 保留，发送方清零，接收方忽略 */
    uint8_t reserved_tail[16]; /* 扩展预留区（云台/机械臂/照明等后置），发送方清零，接收方忽略 */
} TelemetryPacket;

_Static_assert(sizeof(CmdPacket) == 52, "CmdPacket size mismatch");
_Static_assert(sizeof(TelemetryPacket) == 128, "TelemetryPacket size mismatch");

#ifdef __cplusplus
}
#endif

#endif /* ROVI_GENERATED_PROTOCOL_H */