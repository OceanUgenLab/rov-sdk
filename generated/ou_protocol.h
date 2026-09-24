/* AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml */
#ifndef OU_GENERATED_PROTOCOL_H
#define OU_GENERATED_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 帧常量（v0x03：AA 55 | ver | len | type | payload | crc16） */
#define OU_STX0 0xAA
#define OU_STX1 0x55
#define OU_PROTOCOL_VERSION 0x03
#define OU_FRAME_OVERHEAD 7

/* 心跳，双向 1 Hz；armed 判断 = state >= ARMED 且 < CRITICAL */
#define OU_TYPE_HEARTBEAT 0x50
#define OU_HEARTBEAT_PAYLOAD_SIZE 4

/* 系统健康 + 电池 + 链路统计 + 遥测开关，1 Hz */
#define OU_TYPE_SYSSTATUS 0x10
#define OU_SYSSTATUS_PAYLOAD_SIZE 49

/* 命令应答，与 Command 的 command 字段对号 */
#define OU_TYPE_COMMANDACK 0x51
#define OU_COMMANDACK_PAYLOAD_SIZE 8

/* 姿态 + 本地 NED 位置/速度，10–50 Hz；姿态与位置共享时间戳 */
#define OU_TYPE_POSENED 0x13
#define OU_POSENED_PAYLOAD_SIZE 52

/* EKF 健康/方差，1 Hz；方差均为 ×100 编码 */
#define OU_TYPE_EKFSTATUSREPORT 0x14
#define OU_EKFSTATUSREPORT_PAYLOAD_SIZE 7

/* 人工仪表量，1–5 Hz */
#define OU_TYPE_VFRHUD 0x15
#define OU_VFRHUD_PAYLOAD_SIZE 20

/* 全局经纬高 + NED 速度，1 Hz */
#define OU_TYPE_GLOBALPOSITIONINT 0x16
#define OU_GLOBALPOSITIONINT_PAYLOAD_SIZE 28

/* GPS 原始回显，默认关闭，由 CMD_SET_STREAM 开启 */
#define OU_TYPE_GPSRAWINT 0x18
#define OU_GPSRAWINT_PAYLOAD_SIZE 46

/* 水深/距底/水温（Sub 关键帧），1–10 Hz */
#define OU_TYPE_WATERDEPTH 0x19
#define OU_WATERDEPTH_PAYLOAD_SIZE 30

/* 测距（避碰声呐），事件/1 Hz */
#define OU_TYPE_DISTANCESENSOR 0x52
#define OU_DISTANCESENSOR_PAYLOAD_SIZE 23

/* 六轴速度/角速度指令，10–50 Hz；杆量 0 = 该轴交由飞控自稳；sequence 不递增或输入超时（约 500 ms）即 failsafe */
#define OU_TYPE_MANUALCONTROL 0x30
#define OU_MANUALCONTROL_PAYLOAD_SIZE 14

/* 命令帧；飞控以 CommandAck 应答，超时 0.5–1 s 重发 */
#define OU_TYPE_COMMAND 0x32
#define OU_COMMAND_PAYLOAD_SIZE 6

/* 遥控通道回显，默认关闭，由 CMD_SET_STREAM 开启 */
#define OU_TYPE_RCCHANNELS 0x1A
#define OU_RCCHANNELS_PAYLOAD_SIZE 42

/* 执行器 PWM 输出回显，默认关闭，由 CMD_SET_STREAM 开启 */
#define OU_TYPE_SERVOOUTPUTRAW 0x1B
#define OU_SERVOOUTPUTRAW_PAYLOAD_SIZE 37

/* 写参数 */
#define OU_TYPE_PARAMSET 0x33
#define OU_PARAMSET_PAYLOAD_SIZE 21

/* 参数值回读（飞控实际保存值，非请求值回显） */
#define OU_TYPE_PARAMVALUE 0x1D
#define OU_PARAMVALUE_PAYLOAD_SIZE 25

/* Heartbeat 字段偏移 */
#define OU_HEARTBEAT_MODE_OFFSET 0
#define OU_HEARTBEAT_SYSTEM_TYPE_OFFSET 1
#define OU_HEARTBEAT_FW_VERSION_OFFSET 2
#define OU_HEARTBEAT_SYSTEM_STATE_OFFSET 3
/* SysStatus 字段偏移 */
#define OU_SYSSTATUS_SENSORS_PRESENT_OFFSET 0
#define OU_SYSSTATUS_SENSORS_ENABLED_OFFSET 4
#define OU_SYSSTATUS_SENSORS_HEALTH_OFFSET 8
#define OU_SYSSTATUS_LOAD_OFFSET 12
#define OU_SYSSTATUS_VOLTAGE_TOTAL_OFFSET 14
#define OU_SYSSTATUS_VOLTAGE_CELL_MAX_OFFSET 16
#define OU_SYSSTATUS_VOLTAGE_CELL_MIN_OFFSET 18
#define OU_SYSSTATUS_CURRENT_BATTERY_OFFSET 20
#define OU_SYSSTATUS_BATTERY_REMAINING_OFFSET 22
#define OU_SYSSTATUS_CURRENT_CONSUMED_OFFSET 23
#define OU_SYSSTATUS_BATTERY_TEMPERATURE_OFFSET 27
#define OU_SYSSTATUS_BATTERY_FAULT_BITMASK_OFFSET 29
#define OU_SYSSTATUS_DROP_RATE_COMM_OFFSET 33
#define OU_SYSSTATUS_ERRORS_COMM_OFFSET 35
#define OU_SYSSTATUS_ERRORS_COUNT_OFFSET 37
#define OU_SYSSTATUS_STREAM_MASK_OFFSET 45
/* CommandAck 字段偏移 */
#define OU_COMMANDACK_COMMAND_OFFSET 0
#define OU_COMMANDACK_RESULT_OFFSET 2
#define OU_COMMANDACK_PROGRESS_OFFSET 3
#define OU_COMMANDACK_RESULT_PARAM2_OFFSET 4
/* PoseNed 字段偏移 */
#define OU_POSENED_TIME_BOOT_MS_OFFSET 0
#define OU_POSENED_ROLL_OFFSET 4
#define OU_POSENED_PITCH_OFFSET 8
#define OU_POSENED_YAW_OFFSET 12
#define OU_POSENED_ROLLSPEED_OFFSET 16
#define OU_POSENED_PITCHSPEED_OFFSET 20
#define OU_POSENED_YAWSPEED_OFFSET 24
#define OU_POSENED_X_OFFSET 28
#define OU_POSENED_Y_OFFSET 32
#define OU_POSENED_Z_OFFSET 36
#define OU_POSENED_VX_OFFSET 40
#define OU_POSENED_VY_OFFSET 44
#define OU_POSENED_VZ_OFFSET 48
/* EkfStatusReport 字段偏移 */
#define OU_EKFSTATUSREPORT_FLAGS_OFFSET 0
#define OU_EKFSTATUSREPORT_VELOCITY_VARIANCE_OFFSET 2
#define OU_EKFSTATUSREPORT_POS_HORIZ_VARIANCE_OFFSET 3
#define OU_EKFSTATUSREPORT_POS_VERT_VARIANCE_OFFSET 4
#define OU_EKFSTATUSREPORT_COMPASS_VARIANCE_OFFSET 5
#define OU_EKFSTATUSREPORT_TERRAIN_ALT_VARIANCE_OFFSET 6
/* VfrHud 字段偏移 */
#define OU_VFRHUD_AIRSPEED_OFFSET 0
#define OU_VFRHUD_GROUNDSPEED_OFFSET 4
#define OU_VFRHUD_HEADING_OFFSET 8
#define OU_VFRHUD_THROTTLE_OFFSET 10
#define OU_VFRHUD_ALT_OFFSET 12
#define OU_VFRHUD_CLIMB_OFFSET 16
/* GlobalPositionInt 字段偏移 */
#define OU_GLOBALPOSITIONINT_TIME_BOOT_MS_OFFSET 0
#define OU_GLOBALPOSITIONINT_LAT_OFFSET 4
#define OU_GLOBALPOSITIONINT_LON_OFFSET 8
#define OU_GLOBALPOSITIONINT_ALT_OFFSET 12
#define OU_GLOBALPOSITIONINT_RELATIVE_ALT_OFFSET 16
#define OU_GLOBALPOSITIONINT_VX_OFFSET 20
#define OU_GLOBALPOSITIONINT_VY_OFFSET 22
#define OU_GLOBALPOSITIONINT_VZ_OFFSET 24
#define OU_GLOBALPOSITIONINT_HDG_OFFSET 26
/* GpsRawInt 字段偏移 */
#define OU_GPSRAWINT_TIME_USEC_OFFSET 0
#define OU_GPSRAWINT_FIX_TYPE_OFFSET 8
#define OU_GPSRAWINT_LAT_OFFSET 9
#define OU_GPSRAWINT_LON_OFFSET 13
#define OU_GPSRAWINT_ALT_OFFSET 17
#define OU_GPSRAWINT_EPH_OFFSET 21
#define OU_GPSRAWINT_EPV_OFFSET 23
#define OU_GPSRAWINT_VEL_OFFSET 25
#define OU_GPSRAWINT_COG_OFFSET 27
#define OU_GPSRAWINT_SATELLITES_VISIBLE_OFFSET 29
#define OU_GPSRAWINT_H_ACC_OFFSET 30
#define OU_GPSRAWINT_V_ACC_OFFSET 34
#define OU_GPSRAWINT_VEL_ACC_OFFSET 38
#define OU_GPSRAWINT_HDG_ACC_OFFSET 42
/* WaterDepth 字段偏移 */
#define OU_WATERDEPTH_TIME_BOOT_MS_OFFSET 0
#define OU_WATERDEPTH_ID_OFFSET 4
#define OU_WATERDEPTH_HEALTHY_OFFSET 5
#define OU_WATERDEPTH_LAT_OFFSET 6
#define OU_WATERDEPTH_LNG_OFFSET 10
#define OU_WATERDEPTH_ALTITUDE_OFFSET 14
#define OU_WATERDEPTH_BOTTOM_DISTANCE_OFFSET 18
#define OU_WATERDEPTH_TERRAIN_HEIGHT_OFFSET 22
#define OU_WATERDEPTH_TEMPERATURE_OFFSET 26
/* DistanceSensor 字段偏移 */
#define OU_DISTANCESENSOR_TIME_BOOT_MS_OFFSET 0
#define OU_DISTANCESENSOR_MIN_DISTANCE_OFFSET 4
#define OU_DISTANCESENSOR_MAX_DISTANCE_OFFSET 6
#define OU_DISTANCESENSOR_CURRENT_DISTANCE_OFFSET 8
#define OU_DISTANCESENSOR_TYPE_OFFSET 10
#define OU_DISTANCESENSOR_ID_OFFSET 11
#define OU_DISTANCESENSOR_ORIENTATION_OFFSET 12
#define OU_DISTANCESENSOR_COVARIANCE_OFFSET 13
#define OU_DISTANCESENSOR_HORIZONTAL_FOV_OFFSET 14
#define OU_DISTANCESENSOR_VERTICAL_FOV_OFFSET 18
#define OU_DISTANCESENSOR_SIGNAL_QUALITY_OFFSET 22
/* ManualControl 字段偏移 */
#define OU_MANUALCONTROL_SEQUENCE_OFFSET 0
#define OU_MANUALCONTROL_X_OFFSET 2
#define OU_MANUALCONTROL_Y_OFFSET 4
#define OU_MANUALCONTROL_Z_OFFSET 6
#define OU_MANUALCONTROL_P_OFFSET 8
#define OU_MANUALCONTROL_R_OFFSET 10
#define OU_MANUALCONTROL_YAW_OFFSET 12
/* Command 字段偏移 */
#define OU_COMMAND_COMMAND_OFFSET 0
#define OU_COMMAND_PARAM_OFFSET 2
/* RcChannels 字段偏移 */
#define OU_RCCHANNELS_TIME_BOOT_MS_OFFSET 0
#define OU_RCCHANNELS_CHANCOUNT_OFFSET 4
#define OU_RCCHANNELS_CHAN_RAW_OFFSET 5
#define OU_RCCHANNELS_RSSI_OFFSET 41
/* ServoOutputRaw 字段偏移 */
#define OU_SERVOOUTPUTRAW_TIME_BOOT_MS_OFFSET 0
#define OU_SERVOOUTPUTRAW_PORT_OFFSET 4
#define OU_SERVOOUTPUTRAW_SERVO_RAW_OFFSET 5
/* ParamSet 字段偏移 */
#define OU_PARAMSET_PARAM_ID_OFFSET 0
#define OU_PARAMSET_PARAM_VALUE_OFFSET 16
#define OU_PARAMSET_PARAM_TYPE_OFFSET 20
/* ParamValue 字段偏移 */
#define OU_PARAMVALUE_PARAM_ID_OFFSET 0
#define OU_PARAMVALUE_PARAM_VALUE_OFFSET 16
#define OU_PARAMVALUE_PARAM_TYPE_OFFSET 20
#define OU_PARAMVALUE_PARAM_COUNT_OFFSET 21
#define OU_PARAMVALUE_PARAM_INDEX_OFFSET 23

/* Mode 枚举 */
#define OU_MODE_MANUAL 0
#define OU_MODE_AUTO 1
#define OU_MODE_RETURN 2
#define OU_MODE_HOLD 3
/* SystemState 枚举 */
#define OU_SYSTEMSTATE_UNINIT 0
#define OU_SYSTEMSTATE_BOOT 1
#define OU_SYSTEMSTATE_CALIBRATING 2
#define OU_SYSTEMSTATE_STANDBY 3
#define OU_SYSTEMSTATE_ARMED 4
#define OU_SYSTEMSTATE_ACTIVE 5
#define OU_SYSTEMSTATE_CRITICAL 6
#define OU_SYSTEMSTATE_EMERGENCY 7
/* CommandId 枚举 */
#define OU_COMMANDID_CMD_SET_MODE 1
#define OU_COMMANDID_CMD_ARM 2
#define OU_COMMANDID_CMD_GO_HOME 4
#define OU_COMMANDID_CMD_SET_STREAM 5
#define OU_COMMANDID_CMD_SET_PWM 6

/* 心跳，双向 1 Hz；armed 判断 = state >= ARMED 且 < CRITICAL，payload 4 字节 */
typedef struct __attribute__((packed)) {
    uint8_t mode; /* 当前运行模式（权威来源） */
    uint8_t system_type; /* bit0 角色 0=机器人 1=地面站；bit1..7 产品序号 */
    uint8_t fw_version; /* 发送方固件/SDK 主版本号 */
    uint8_t system_state; /* SystemState 枚举 */
} Heartbeat;

_Static_assert(sizeof(Heartbeat) == 4, "Heartbeat size mismatch");

/* 系统健康 + 电池 + 链路统计 + 遥测开关，1 Hz，payload 49 字节 */
typedef struct __attribute__((packed)) {
    uint32_t sensors_present; /* 已安装传感器位图 */
    uint32_t sensors_enabled; /* 已启用传感器位图 */
    uint32_t sensors_health; /* 传感器健康位图 */
    uint16_t load; /* CPU 负载 %×10 */
    uint16_t voltage_total; /* 电池总电压，65535=未知 */
    uint16_t voltage_cell_max; /* 单体最高电压，65535=未知 */
    uint16_t voltage_cell_min; /* 单体最低电压，65535=未知 */
    int16_t current_battery; /* 电池电流，-1=未知 */
    int8_t battery_remaining; /* 剩余百分比，-1=未知 */
    int32_t current_consumed; /* 已消耗容量，-1=未知 */
    int16_t battery_temperature; /* 电池温度 °C×100 */
    uint32_t battery_fault_bitmask; /* 电池故障位图 */
    uint16_t drop_rate_comm; /* 丢包率 %×100 */
    uint16_t errors_comm; /* 通信错误计数 */
    uint16_t errors_count[4]; /* 系统错误计数 */
    uint32_t stream_mask; /* 回显帧开关位图 bit0=GpsRawInt bit1=RcChannels bit2=ServoOutputRaw */
} SysStatus;

_Static_assert(sizeof(SysStatus) == 49, "SysStatus size mismatch");

/* 命令应答，与 Command 的 command 字段对号，payload 8 字节 */
typedef struct __attribute__((packed)) {
    uint16_t command; /* 被确认的命令号（Command 枚举） */
    uint8_t result; /* 0 接受 1 暂时拒绝 2 拒绝 3 不支持 4 失败 5 进行中 */
    uint8_t progress; /* 进度百分比 */
    int32_t result_param2; /* 附加结果参数 */
} CommandAck;

_Static_assert(sizeof(CommandAck) == 8, "CommandAck size mismatch");

/* 姿态 + 本地 NED 位置/速度，10–50 Hz；姿态与位置共享时间戳，payload 52 字节 */
typedef struct __attribute__((packed)) {
    uint32_t time_boot_ms; /* 开机毫秒时间戳 */
    float roll; /* 横滚角 */
    float pitch; /* 俯仰角 */
    float yaw; /* 偏航角 */
    float rollspeed; /* 横滚角速度 */
    float pitchspeed; /* 俯仰角速度 */
    float yawspeed; /* 偏航角速度 */
    float x; /* 北向位置（NED） */
    float y; /* 东向位置（NED） */
    float z; /* 下向位置（NED） */
    float vx; /* 北向速度 */
    float vy; /* 东向速度 */
    float vz; /* 下向速度 */
} PoseNed;

_Static_assert(sizeof(PoseNed) == 52, "PoseNed size mismatch");

/* EKF 健康/方差，1 Hz；方差均为 ×100 编码，payload 7 字节 */
typedef struct __attribute__((packed)) {
    uint16_t flags; /* EKF 健康/融合状态位 */
    uint8_t velocity_variance; /* 速度方差×100 */
    uint8_t pos_horiz_variance; /* 水平位置方差×100 */
    uint8_t pos_vert_variance; /* 垂直位置方差×100 */
    uint8_t compass_variance; /* 罗盘方差×100 */
    uint8_t terrain_alt_variance; /* 地形高度方差×100 */
} EkfStatusReport;

_Static_assert(sizeof(EkfStatusReport) == 7, "EkfStatusReport size mismatch");

/* 人工仪表量，1–5 Hz，payload 20 字节 */
typedef struct __attribute__((packed)) {
    float airspeed; /* 水航速（无传感器填 0） */
    float groundspeed; /* 对地速度 */
    int16_t heading; /* 航向角 */
    uint16_t throttle; /* 油门档位 */
    float alt; /* 距底高度（与 WaterDepth.altitude 同义） */
    float climb; /* 垂直速度，上浮为正 */
} VfrHud;

_Static_assert(sizeof(VfrHud) == 20, "VfrHud size mismatch");

/* 全局经纬高 + NED 速度，1 Hz，payload 28 字节 */
typedef struct __attribute__((packed)) {
    uint32_t time_boot_ms; /* 开机毫秒时间戳 */
    int32_t lat; /* 纬度×1e7 */
    int32_t lon; /* 经度×1e7 */
    int32_t alt; /* 海拔高度 AMSL */
    int32_t relative_alt; /* 相对 Home 高度 */
    int16_t vx; /* 北向速度 */
    int16_t vy; /* 东向速度 */
    int16_t vz; /* 下向速度 */
    uint16_t hdg; /* 航向×100，65535=未知 */
} GlobalPositionInt;

_Static_assert(sizeof(GlobalPositionInt) == 28, "GlobalPositionInt size mismatch");

/* GPS 原始回显，默认关闭，由 CMD_SET_STREAM 开启，payload 46 字节 */
typedef struct __attribute__((packed)) {
    uint64_t time_usec; /* us 级时间戳 */
    uint8_t fix_type; /* 定位类型枚举 */
    int32_t lat; /* 纬度×1e7 */
    int32_t lon; /* 经度×1e7 */
    int32_t alt; /* 海拔高度 */
    uint16_t eph; /* 水平精度×100，65535=未知 */
    uint16_t epv; /* 垂直精度×100，65535=未知 */
    uint16_t vel; /* 地速 */
    uint16_t cog; /* 航向×100 */
    uint8_t satellites_visible; /* 可见卫星数 */
    uint32_t h_acc; /* 水平精度 */
    uint32_t v_acc; /* 垂直精度 */
    uint32_t vel_acc; /* 速度精度 */
    uint32_t hdg_acc; /* 航向精度 */
} GpsRawInt;

_Static_assert(sizeof(GpsRawInt) == 46, "GpsRawInt size mismatch");

/* 水深/距底/水温（Sub 关键帧），1–10 Hz，payload 30 字节 */
typedef struct __attribute__((packed)) {
    uint32_t time_boot_ms; /* 开机毫秒时间戳 */
    uint8_t id; /* 传感器 ID */
    uint8_t healthy; /* 健康状态 */
    int32_t lat; /* 纬度×1e7，可选 */
    int32_t lng; /* 经度×1e7，可选 */
    float altitude; /* 距底高度 */
    float bottom_distance; /* 到水底距离 */
    float terrain_height; /* 地形高度 */
    float temperature; /* 水温 */
} WaterDepth;

_Static_assert(sizeof(WaterDepth) == 30, "WaterDepth size mismatch");

/* 测距（避碰声呐），事件/1 Hz，payload 23 字节 */
typedef struct __attribute__((packed)) {
    uint32_t time_boot_ms; /* 开机毫秒时间戳 */
    uint16_t min_distance; /* 量程下限 */
    uint16_t max_distance; /* 量程上限 */
    uint16_t current_distance; /* 当前距离 */
    uint8_t type; /* 测距类型 */
    uint8_t id; /* 传感器 ID */
    uint8_t orientation; /* 朝向 */
    uint8_t covariance; /* 协方差，255=未知 */
    float horizontal_fov; /* 水平视场角 */
    float vertical_fov; /* 垂直视场角 */
    uint8_t signal_quality; /* 信号质量，255=未知 */
} DistanceSensor;

_Static_assert(sizeof(DistanceSensor) == 23, "DistanceSensor size mismatch");

/* 六轴速度/角速度指令，10–50 Hz；杆量 0 = 该轴交由飞控自稳；sequence 不递增或输入超时（约 500 ms）即 failsafe，payload 14 字节 */
typedef struct __attribute__((packed)) {
    uint16_t sequence; /* 递增序号，丢包/乱序检测 */
    int16_t x; /* 前后速度 surge */
    int16_t y; /* 横移速度 sway */
    int16_t z; /* 升沉速度 heave */
    int16_t p; /* 俯仰角速度 pitch rate */
    int16_t r; /* 横滚角速度 roll rate */
    int16_t yaw; /* 偏航角速度 yaw rate */
} ManualControl;

_Static_assert(sizeof(ManualControl) == 14, "ManualControl size mismatch");

/* 命令帧；飞控以 CommandAck 应答，超时 0.5–1 s 重发，payload 6 字节 */
typedef struct __attribute__((packed)) {
    uint16_t command; /* Command 枚举 */
    uint32_t param; /* 命令参数，按命令定义解释 */
} Command;

_Static_assert(sizeof(Command) == 6, "Command size mismatch");

/* 遥控通道回显，默认关闭，由 CMD_SET_STREAM 开启，payload 42 字节 */
typedef struct __attribute__((packed)) {
    uint32_t time_boot_ms; /* 开机毫秒时间戳 */
    uint8_t chancount; /* 有效通道数 */
    uint16_t chan_raw[18]; /* 18 路通道原始值，65535=无效 */
    uint8_t rssi; /* 遥控链路强度，255=未知 */
} RcChannels;

_Static_assert(sizeof(RcChannels) == 42, "RcChannels size mismatch");

/* 执行器 PWM 输出回显，默认关闭，由 CMD_SET_STREAM 开启，payload 37 字节 */
typedef struct __attribute__((packed)) {
    uint32_t time_boot_ms; /* ms 级时间戳 */
    uint8_t port; /* 输出端口 */
    uint16_t servo_raw[16]; /* 16 路 PWM 输出 */
} ServoOutputRaw;

_Static_assert(sizeof(ServoOutputRaw) == 37, "ServoOutputRaw size mismatch");

/* 写参数，payload 21 字节 */
typedef struct __attribute__((packed)) {
    char param_id[16]; /* 参数名 */
    float param_value; /* 目标值 */
    uint8_t param_type; /* OU 类型枚举 */
} ParamSet;

_Static_assert(sizeof(ParamSet) == 21, "ParamSet size mismatch");

/* 参数值回读（飞控实际保存值，非请求值回显），payload 25 字节 */
typedef struct __attribute__((packed)) {
    char param_id[16]; /* 参数名 */
    float param_value; /* 实际保存值 */
    uint8_t param_type; /* OU 类型枚举 */
    uint16_t param_count; /* 参数总数 */
    uint16_t param_index; /* 当前序号 */
} ParamValue;

_Static_assert(sizeof(ParamValue) == 25, "ParamValue size mismatch");


#ifdef __cplusplus
}
#endif

#endif /* OU_GENERATED_PROTOCOL_H */