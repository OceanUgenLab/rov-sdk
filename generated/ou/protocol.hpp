// AUTO-GENERATED, DO NOT EDIT, source: schema/protocol.yaml

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

namespace ou {

namespace detail {

// 字段访问器：标量直接返回，数组取首元素指针（用于生成器统一展开）
template <typename T, size_t N>
inline constexpr const T* field_ptr(const T (&v)[N]) { return v; }
template <typename T>
inline constexpr const T* field_ptr(const T& v) { return &v; }
template <typename T, size_t N>
inline constexpr T* field_ptr(T (&v)[N]) { return v; }
template <typename T>
inline constexpr T* field_ptr(T& v) { return &v; }
template <typename T, size_t N>
inline constexpr size_t field_count(const T (&)[N]) { return N; }
template <typename T>
inline constexpr size_t field_count(const T&) { return 1; }

// 小端字节读写（无 strict-aliasing UB）
inline void putBytes(std::vector<uint8_t>& out, const uint8_t* p, size_t n) {
    out.insert(out.end(), p, p + n);
}
inline uint64_t getUintLE(std::span<const uint8_t> s, size_t off, size_t n) {
    uint64_t v = 0;
    for (size_t i = 0; i < n; ++i) v |= static_cast<uint64_t>(s[off + i]) << (8 * i);
    return v;
}

}  // namespace detail

// CRC-16/MODBUS（init 0xFFFF, poly 0xA001 反射），实现在 src/protocol.cpp
uint16_t crc16(std::span<const uint8_t> data);

// ---------------------------------------------------------------------------
// 帧常量（v0x03 帧格式：AA 55 | ver | len | type | payload | crc16）
// ---------------------------------------------------------------------------
inline constexpr uint8_t kStx0 = 0xAA;
inline constexpr uint8_t kStx1 = 0x55;
inline constexpr uint8_t kProtocolVersion = 0x03;
inline constexpr size_t kFrameOverhead = 7;  // STX(2)+ver(1)+len(1)+type(1)+crc(2)

// 心跳，双向 1 Hz；armed 判断 = state >= ARMED 且 < CRITICAL
inline constexpr uint8_t kTypeHeartbeat = 0x50;
inline constexpr size_t kHeartbeatPayloadSize = 4;
// 系统健康 + 电池 + 链路统计 + 遥测开关，1 Hz
inline constexpr uint8_t kTypeSysStatus = 0x10;
inline constexpr size_t kSysStatusPayloadSize = 49;
// 命令应答，与 Command 的 command 字段对号
inline constexpr uint8_t kTypeCommandAck = 0x51;
inline constexpr size_t kCommandAckPayloadSize = 8;
// 姿态 + 本地 NED 位置/速度，10–50 Hz；姿态与位置共享时间戳
inline constexpr uint8_t kTypePoseNed = 0x13;
inline constexpr size_t kPoseNedPayloadSize = 52;
// EKF 健康/方差，1 Hz；方差均为 ×100 编码
inline constexpr uint8_t kTypeEkfStatusReport = 0x14;
inline constexpr size_t kEkfStatusReportPayloadSize = 7;
// 人工仪表量，1–5 Hz
inline constexpr uint8_t kTypeVfrHud = 0x15;
inline constexpr size_t kVfrHudPayloadSize = 20;
// 全局经纬高 + NED 速度，1 Hz
inline constexpr uint8_t kTypeGlobalPositionInt = 0x16;
inline constexpr size_t kGlobalPositionIntPayloadSize = 28;
// GPS 原始回显，默认关闭，由 CMD_SET_STREAM 开启
inline constexpr uint8_t kTypeGpsRawInt = 0x18;
inline constexpr size_t kGpsRawIntPayloadSize = 46;
// 水深/距底/水温（Sub 关键帧），1–10 Hz
inline constexpr uint8_t kTypeWaterDepth = 0x19;
inline constexpr size_t kWaterDepthPayloadSize = 30;
// 测距（避碰声呐），事件/1 Hz
inline constexpr uint8_t kTypeDistanceSensor = 0x52;
inline constexpr size_t kDistanceSensorPayloadSize = 23;
// 六轴速度/角速度指令，10–50 Hz；杆量 0 = 该轴交由飞控自稳；sequence 不递增或输入超时（约 500 ms）即 failsafe
inline constexpr uint8_t kTypeManualControl = 0x30;
inline constexpr size_t kManualControlPayloadSize = 14;
// 命令帧；飞控以 CommandAck 应答，超时 0.5–1 s 重发
inline constexpr uint8_t kTypeCommand = 0x32;
inline constexpr size_t kCommandPayloadSize = 6;
// 遥控通道回显，默认关闭，由 CMD_SET_STREAM 开启
inline constexpr uint8_t kTypeRcChannels = 0x1A;
inline constexpr size_t kRcChannelsPayloadSize = 42;
// 执行器 PWM 输出回显，默认关闭，由 CMD_SET_STREAM 开启
inline constexpr uint8_t kTypeServoOutputRaw = 0x1B;
inline constexpr size_t kServoOutputRawPayloadSize = 37;
// 写参数
inline constexpr uint8_t kTypeParamSet = 0x33;
inline constexpr size_t kParamSetPayloadSize = 21;
// 参数值回读（飞控实际保存值，非请求值回显）
inline constexpr uint8_t kTypeParamValue = 0x1D;
inline constexpr size_t kParamValuePayloadSize = 25;

// ---------------------------------------------------------------------------
// 枚举常量
// ---------------------------------------------------------------------------
enum class Mode : uint8_t {
    kManual = 0,
    kAuto = 1,
    kReturn = 2,
    kHold = 3,
};
enum class SystemState : uint8_t {
    kUninit = 0,
    kBoot = 1,
    kCalibrating = 2,
    kStandby = 3,
    kArmed = 4,
    kActive = 5,
    kCritical = 6,
    kEmergency = 7,
};
enum class CommandId : uint16_t {
    kCmdSetMode = 1,
    kCmdArm = 2,
    kCmdGoHome = 4,
    kCmdSetStream = 5,
    kCmdSetPwm = 6,
};

// ---------------------------------------------------------------------------
// 载荷结构体
// ---------------------------------------------------------------------------
// 心跳，双向 1 Hz；armed 判断 = state >= ARMED 且 < CRITICAL，payload 4 字节
struct Heartbeat {
    uint8_t mode{}; // 当前运行模式（权威来源）
    uint8_t system_type{}; // bit0 角色 0=机器人 1=地面站；bit1..7 产品序号
    uint8_t fw_version{}; // 发送方固件/SDK 主版本号
    uint8_t system_state{}; // SystemState 枚举

    friend bool operator==(const Heartbeat&, const Heartbeat&) = default;
};

// 系统健康 + 电池 + 链路统计 + 遥测开关，1 Hz，payload 49 字节
struct SysStatus {
    uint32_t sensors_present{}; // 已安装传感器位图
    uint32_t sensors_enabled{}; // 已启用传感器位图
    uint32_t sensors_health{}; // 传感器健康位图
    uint16_t load{}; // CPU 负载 %×10
    uint16_t voltage_total{}; // 电池总电压，65535=未知
    uint16_t voltage_cell_max{}; // 单体最高电压，65535=未知
    uint16_t voltage_cell_min{}; // 单体最低电压，65535=未知
    int16_t current_battery{}; // 电池电流，-1=未知
    int8_t battery_remaining{}; // 剩余百分比，-1=未知
    int32_t current_consumed{}; // 已消耗容量，-1=未知
    int16_t battery_temperature{}; // 电池温度 °C×100
    uint32_t battery_fault_bitmask{}; // 电池故障位图
    uint16_t drop_rate_comm{}; // 丢包率 %×100
    uint16_t errors_comm{}; // 通信错误计数
    uint16_t errors_count[4]{}; // 系统错误计数
    uint32_t stream_mask{}; // 回显帧开关位图 bit0=GpsRawInt bit1=RcChannels bit2=ServoOutputRaw

    friend bool operator==(const SysStatus&, const SysStatus&) = default;
};

// 命令应答，与 Command 的 command 字段对号，payload 8 字节
struct CommandAck {
    uint16_t command{}; // 被确认的命令号（Command 枚举）
    uint8_t result{}; // 0 接受 1 暂时拒绝 2 拒绝 3 不支持 4 失败 5 进行中
    uint8_t progress{}; // 进度百分比
    int32_t result_param2{}; // 附加结果参数

    friend bool operator==(const CommandAck&, const CommandAck&) = default;
};

// 姿态 + 本地 NED 位置/速度，10–50 Hz；姿态与位置共享时间戳，payload 52 字节
struct PoseNed {
    uint32_t time_boot_ms{}; // 开机毫秒时间戳
    float roll{}; // 横滚角
    float pitch{}; // 俯仰角
    float yaw{}; // 偏航角
    float rollspeed{}; // 横滚角速度
    float pitchspeed{}; // 俯仰角速度
    float yawspeed{}; // 偏航角速度
    float x{}; // 北向位置（NED）
    float y{}; // 东向位置（NED）
    float z{}; // 下向位置（NED）
    float vx{}; // 北向速度
    float vy{}; // 东向速度
    float vz{}; // 下向速度

    friend bool operator==(const PoseNed&, const PoseNed&) = default;
};

// EKF 健康/方差，1 Hz；方差均为 ×100 编码，payload 7 字节
struct EkfStatusReport {
    uint16_t flags{}; // EKF 健康/融合状态位
    uint8_t velocity_variance{}; // 速度方差×100
    uint8_t pos_horiz_variance{}; // 水平位置方差×100
    uint8_t pos_vert_variance{}; // 垂直位置方差×100
    uint8_t compass_variance{}; // 罗盘方差×100
    uint8_t terrain_alt_variance{}; // 地形高度方差×100

    friend bool operator==(const EkfStatusReport&, const EkfStatusReport&) = default;
};

// 人工仪表量，1–5 Hz，payload 20 字节
struct VfrHud {
    float airspeed{}; // 水航速（无传感器填 0）
    float groundspeed{}; // 对地速度
    int16_t heading{}; // 航向角
    uint16_t throttle{}; // 油门档位
    float alt{}; // 距底高度（与 WaterDepth.altitude 同义）
    float climb{}; // 垂直速度，上浮为正

    friend bool operator==(const VfrHud&, const VfrHud&) = default;
};

// 全局经纬高 + NED 速度，1 Hz，payload 28 字节
struct GlobalPositionInt {
    uint32_t time_boot_ms{}; // 开机毫秒时间戳
    int32_t lat{}; // 纬度×1e7
    int32_t lon{}; // 经度×1e7
    int32_t alt{}; // 海拔高度 AMSL
    int32_t relative_alt{}; // 相对 Home 高度
    int16_t vx{}; // 北向速度
    int16_t vy{}; // 东向速度
    int16_t vz{}; // 下向速度
    uint16_t hdg{}; // 航向×100，65535=未知

    friend bool operator==(const GlobalPositionInt&, const GlobalPositionInt&) = default;
};

// GPS 原始回显，默认关闭，由 CMD_SET_STREAM 开启，payload 46 字节
struct GpsRawInt {
    uint64_t time_usec{}; // us 级时间戳
    uint8_t fix_type{}; // 定位类型枚举
    int32_t lat{}; // 纬度×1e7
    int32_t lon{}; // 经度×1e7
    int32_t alt{}; // 海拔高度
    uint16_t eph{}; // 水平精度×100，65535=未知
    uint16_t epv{}; // 垂直精度×100，65535=未知
    uint16_t vel{}; // 地速
    uint16_t cog{}; // 航向×100
    uint8_t satellites_visible{}; // 可见卫星数
    uint32_t h_acc{}; // 水平精度
    uint32_t v_acc{}; // 垂直精度
    uint32_t vel_acc{}; // 速度精度
    uint32_t hdg_acc{}; // 航向精度

    friend bool operator==(const GpsRawInt&, const GpsRawInt&) = default;
};

// 水深/距底/水温（Sub 关键帧），1–10 Hz，payload 30 字节
struct WaterDepth {
    uint32_t time_boot_ms{}; // 开机毫秒时间戳
    uint8_t id{}; // 传感器 ID
    uint8_t healthy{}; // 健康状态
    int32_t lat{}; // 纬度×1e7，可选
    int32_t lng{}; // 经度×1e7，可选
    float altitude{}; // 距底高度
    float bottom_distance{}; // 到水底距离
    float terrain_height{}; // 地形高度
    float temperature{}; // 水温

    friend bool operator==(const WaterDepth&, const WaterDepth&) = default;
};

// 测距（避碰声呐），事件/1 Hz，payload 23 字节
struct DistanceSensor {
    uint32_t time_boot_ms{}; // 开机毫秒时间戳
    uint16_t min_distance{}; // 量程下限
    uint16_t max_distance{}; // 量程上限
    uint16_t current_distance{}; // 当前距离
    uint8_t type{}; // 测距类型
    uint8_t id{}; // 传感器 ID
    uint8_t orientation{}; // 朝向
    uint8_t covariance{}; // 协方差，255=未知
    float horizontal_fov{}; // 水平视场角
    float vertical_fov{}; // 垂直视场角
    uint8_t signal_quality{}; // 信号质量，255=未知

    friend bool operator==(const DistanceSensor&, const DistanceSensor&) = default;
};

// 六轴速度/角速度指令，10–50 Hz；杆量 0 = 该轴交由飞控自稳；sequence 不递增或输入超时（约 500 ms）即 failsafe，payload 14 字节
struct ManualControl {
    uint16_t sequence{}; // 递增序号，丢包/乱序检测
    int16_t x{}; // 前后速度 surge
    int16_t y{}; // 横移速度 sway
    int16_t z{}; // 升沉速度 heave
    int16_t p{}; // 俯仰角速度 pitch rate
    int16_t r{}; // 横滚角速度 roll rate
    int16_t yaw{}; // 偏航角速度 yaw rate

    friend bool operator==(const ManualControl&, const ManualControl&) = default;
};

// 命令帧；飞控以 CommandAck 应答，超时 0.5–1 s 重发，payload 6 字节
struct Command {
    uint16_t command{}; // Command 枚举
    uint32_t param{}; // 命令参数，按命令定义解释

    friend bool operator==(const Command&, const Command&) = default;
};

// 遥控通道回显，默认关闭，由 CMD_SET_STREAM 开启，payload 42 字节
struct RcChannels {
    uint32_t time_boot_ms{}; // 开机毫秒时间戳
    uint8_t chancount{}; // 有效通道数
    uint16_t chan_raw[18]{}; // 18 路通道原始值，65535=无效
    uint8_t rssi{}; // 遥控链路强度，255=未知

    friend bool operator==(const RcChannels&, const RcChannels&) = default;
};

// 执行器 PWM 输出回显，默认关闭，由 CMD_SET_STREAM 开启，payload 37 字节
struct ServoOutputRaw {
    uint32_t time_boot_ms{}; // ms 级时间戳
    uint8_t port{}; // 输出端口
    uint16_t servo_raw[16]{}; // 16 路 PWM 输出

    friend bool operator==(const ServoOutputRaw&, const ServoOutputRaw&) = default;
};

// 写参数，payload 21 字节
struct ParamSet {
    char param_id[16]{}; // 参数名
    float param_value{}; // 目标值
    uint8_t param_type{}; // OU 类型枚举

    friend bool operator==(const ParamSet&, const ParamSet&) = default;
};

// 参数值回读（飞控实际保存值，非请求值回显），payload 25 字节
struct ParamValue {
    char param_id[16]{}; // 参数名
    float param_value{}; // 实际保存值
    uint8_t param_type{}; // OU 类型枚举
    uint16_t param_count{}; // 参数总数
    uint16_t param_index{}; // 当前序号

    friend bool operator==(const ParamValue&, const ParamValue&) = default;
};


// ---------------------------------------------------------------------------
// 字段偏移常量（从字段顺序+类型尺寸推导，单位字节）
// ---------------------------------------------------------------------------
inline constexpr size_t kHeartbeatmodeOffset = 0; // Heartbeat.mode
inline constexpr size_t kHeartbeatSystemTypeOffset = 1; // Heartbeat.system_type
inline constexpr size_t kHeartbeatFwVersionOffset = 2; // Heartbeat.fw_version
inline constexpr size_t kHeartbeatSystemStateOffset = 3; // Heartbeat.system_state
inline constexpr size_t kSysStatusSensorsPresentOffset = 0; // SysStatus.sensors_present
inline constexpr size_t kSysStatusSensorsEnabledOffset = 4; // SysStatus.sensors_enabled
inline constexpr size_t kSysStatusSensorsHealthOffset = 8; // SysStatus.sensors_health
inline constexpr size_t kSysStatusloadOffset = 12; // SysStatus.load
inline constexpr size_t kSysStatusVoltageTotalOffset = 14; // SysStatus.voltage_total
inline constexpr size_t kSysStatusVoltageCellMaxOffset = 16; // SysStatus.voltage_cell_max
inline constexpr size_t kSysStatusVoltageCellMinOffset = 18; // SysStatus.voltage_cell_min
inline constexpr size_t kSysStatusCurrentBatteryOffset = 20; // SysStatus.current_battery
inline constexpr size_t kSysStatusBatteryRemainingOffset = 22; // SysStatus.battery_remaining
inline constexpr size_t kSysStatusCurrentConsumedOffset = 23; // SysStatus.current_consumed
inline constexpr size_t kSysStatusBatteryTemperatureOffset = 27; // SysStatus.battery_temperature
inline constexpr size_t kSysStatusBatteryFaultBitmaskOffset = 29; // SysStatus.battery_fault_bitmask
inline constexpr size_t kSysStatusDropRateCommOffset = 33; // SysStatus.drop_rate_comm
inline constexpr size_t kSysStatusErrorsCommOffset = 35; // SysStatus.errors_comm
inline constexpr size_t kSysStatusErrorsCountOffset = 37; // SysStatus.errors_count
inline constexpr size_t kSysStatusStreamMaskOffset = 45; // SysStatus.stream_mask
inline constexpr size_t kCommandAckcommandOffset = 0; // CommandAck.command
inline constexpr size_t kCommandAckresultOffset = 2; // CommandAck.result
inline constexpr size_t kCommandAckprogressOffset = 3; // CommandAck.progress
inline constexpr size_t kCommandAckResultParam2Offset = 4; // CommandAck.result_param2
inline constexpr size_t kPoseNedTimeBootMsOffset = 0; // PoseNed.time_boot_ms
inline constexpr size_t kPoseNedrollOffset = 4; // PoseNed.roll
inline constexpr size_t kPoseNedpitchOffset = 8; // PoseNed.pitch
inline constexpr size_t kPoseNedyawOffset = 12; // PoseNed.yaw
inline constexpr size_t kPoseNedrollspeedOffset = 16; // PoseNed.rollspeed
inline constexpr size_t kPoseNedpitchspeedOffset = 20; // PoseNed.pitchspeed
inline constexpr size_t kPoseNedyawspeedOffset = 24; // PoseNed.yawspeed
inline constexpr size_t kPoseNedxOffset = 28; // PoseNed.x
inline constexpr size_t kPoseNedyOffset = 32; // PoseNed.y
inline constexpr size_t kPoseNedzOffset = 36; // PoseNed.z
inline constexpr size_t kPoseNedvxOffset = 40; // PoseNed.vx
inline constexpr size_t kPoseNedvyOffset = 44; // PoseNed.vy
inline constexpr size_t kPoseNedvzOffset = 48; // PoseNed.vz
inline constexpr size_t kEkfStatusReportflagsOffset = 0; // EkfStatusReport.flags
inline constexpr size_t kEkfStatusReportVelocityVarianceOffset = 2; // EkfStatusReport.velocity_variance
inline constexpr size_t kEkfStatusReportPosHorizVarianceOffset = 3; // EkfStatusReport.pos_horiz_variance
inline constexpr size_t kEkfStatusReportPosVertVarianceOffset = 4; // EkfStatusReport.pos_vert_variance
inline constexpr size_t kEkfStatusReportCompassVarianceOffset = 5; // EkfStatusReport.compass_variance
inline constexpr size_t kEkfStatusReportTerrainAltVarianceOffset = 6; // EkfStatusReport.terrain_alt_variance
inline constexpr size_t kVfrHudairspeedOffset = 0; // VfrHud.airspeed
inline constexpr size_t kVfrHudgroundspeedOffset = 4; // VfrHud.groundspeed
inline constexpr size_t kVfrHudheadingOffset = 8; // VfrHud.heading
inline constexpr size_t kVfrHudthrottleOffset = 10; // VfrHud.throttle
inline constexpr size_t kVfrHudaltOffset = 12; // VfrHud.alt
inline constexpr size_t kVfrHudclimbOffset = 16; // VfrHud.climb
inline constexpr size_t kGlobalPositionIntTimeBootMsOffset = 0; // GlobalPositionInt.time_boot_ms
inline constexpr size_t kGlobalPositionIntlatOffset = 4; // GlobalPositionInt.lat
inline constexpr size_t kGlobalPositionIntlonOffset = 8; // GlobalPositionInt.lon
inline constexpr size_t kGlobalPositionIntaltOffset = 12; // GlobalPositionInt.alt
inline constexpr size_t kGlobalPositionIntRelativeAltOffset = 16; // GlobalPositionInt.relative_alt
inline constexpr size_t kGlobalPositionIntvxOffset = 20; // GlobalPositionInt.vx
inline constexpr size_t kGlobalPositionIntvyOffset = 22; // GlobalPositionInt.vy
inline constexpr size_t kGlobalPositionIntvzOffset = 24; // GlobalPositionInt.vz
inline constexpr size_t kGlobalPositionInthdgOffset = 26; // GlobalPositionInt.hdg
inline constexpr size_t kGpsRawIntTimeUsecOffset = 0; // GpsRawInt.time_usec
inline constexpr size_t kGpsRawIntFixTypeOffset = 8; // GpsRawInt.fix_type
inline constexpr size_t kGpsRawIntlatOffset = 9; // GpsRawInt.lat
inline constexpr size_t kGpsRawIntlonOffset = 13; // GpsRawInt.lon
inline constexpr size_t kGpsRawIntaltOffset = 17; // GpsRawInt.alt
inline constexpr size_t kGpsRawIntephOffset = 21; // GpsRawInt.eph
inline constexpr size_t kGpsRawIntepvOffset = 23; // GpsRawInt.epv
inline constexpr size_t kGpsRawIntvelOffset = 25; // GpsRawInt.vel
inline constexpr size_t kGpsRawIntcogOffset = 27; // GpsRawInt.cog
inline constexpr size_t kGpsRawIntSatellitesVisibleOffset = 29; // GpsRawInt.satellites_visible
inline constexpr size_t kGpsRawIntHAccOffset = 30; // GpsRawInt.h_acc
inline constexpr size_t kGpsRawIntVAccOffset = 34; // GpsRawInt.v_acc
inline constexpr size_t kGpsRawIntVelAccOffset = 38; // GpsRawInt.vel_acc
inline constexpr size_t kGpsRawIntHdgAccOffset = 42; // GpsRawInt.hdg_acc
inline constexpr size_t kWaterDepthTimeBootMsOffset = 0; // WaterDepth.time_boot_ms
inline constexpr size_t kWaterDepthidOffset = 4; // WaterDepth.id
inline constexpr size_t kWaterDepthhealthyOffset = 5; // WaterDepth.healthy
inline constexpr size_t kWaterDepthlatOffset = 6; // WaterDepth.lat
inline constexpr size_t kWaterDepthlngOffset = 10; // WaterDepth.lng
inline constexpr size_t kWaterDepthaltitudeOffset = 14; // WaterDepth.altitude
inline constexpr size_t kWaterDepthBottomDistanceOffset = 18; // WaterDepth.bottom_distance
inline constexpr size_t kWaterDepthTerrainHeightOffset = 22; // WaterDepth.terrain_height
inline constexpr size_t kWaterDepthtemperatureOffset = 26; // WaterDepth.temperature
inline constexpr size_t kDistanceSensorTimeBootMsOffset = 0; // DistanceSensor.time_boot_ms
inline constexpr size_t kDistanceSensorMinDistanceOffset = 4; // DistanceSensor.min_distance
inline constexpr size_t kDistanceSensorMaxDistanceOffset = 6; // DistanceSensor.max_distance
inline constexpr size_t kDistanceSensorCurrentDistanceOffset = 8; // DistanceSensor.current_distance
inline constexpr size_t kDistanceSensortypeOffset = 10; // DistanceSensor.type
inline constexpr size_t kDistanceSensoridOffset = 11; // DistanceSensor.id
inline constexpr size_t kDistanceSensororientationOffset = 12; // DistanceSensor.orientation
inline constexpr size_t kDistanceSensorcovarianceOffset = 13; // DistanceSensor.covariance
inline constexpr size_t kDistanceSensorHorizontalFovOffset = 14; // DistanceSensor.horizontal_fov
inline constexpr size_t kDistanceSensorVerticalFovOffset = 18; // DistanceSensor.vertical_fov
inline constexpr size_t kDistanceSensorSignalQualityOffset = 22; // DistanceSensor.signal_quality
inline constexpr size_t kManualControlsequenceOffset = 0; // ManualControl.sequence
inline constexpr size_t kManualControlxOffset = 2; // ManualControl.x
inline constexpr size_t kManualControlyOffset = 4; // ManualControl.y
inline constexpr size_t kManualControlzOffset = 6; // ManualControl.z
inline constexpr size_t kManualControlpOffset = 8; // ManualControl.p
inline constexpr size_t kManualControlrOffset = 10; // ManualControl.r
inline constexpr size_t kManualControlyawOffset = 12; // ManualControl.yaw
inline constexpr size_t kCommandcommandOffset = 0; // Command.command
inline constexpr size_t kCommandparamOffset = 2; // Command.param
inline constexpr size_t kRcChannelsTimeBootMsOffset = 0; // RcChannels.time_boot_ms
inline constexpr size_t kRcChannelschancountOffset = 4; // RcChannels.chancount
inline constexpr size_t kRcChannelsChanRawOffset = 5; // RcChannels.chan_raw
inline constexpr size_t kRcChannelsrssiOffset = 41; // RcChannels.rssi
inline constexpr size_t kServoOutputRawTimeBootMsOffset = 0; // ServoOutputRaw.time_boot_ms
inline constexpr size_t kServoOutputRawportOffset = 4; // ServoOutputRaw.port
inline constexpr size_t kServoOutputRawServoRawOffset = 5; // ServoOutputRaw.servo_raw
inline constexpr size_t kParamSetParamIdOffset = 0; // ParamSet.param_id
inline constexpr size_t kParamSetParamValueOffset = 16; // ParamSet.param_value
inline constexpr size_t kParamSetParamTypeOffset = 20; // ParamSet.param_type
inline constexpr size_t kParamValueParamIdOffset = 0; // ParamValue.param_id
inline constexpr size_t kParamValueParamValueOffset = 16; // ParamValue.param_value
inline constexpr size_t kParamValueParamTypeOffset = 20; // ParamValue.param_type
inline constexpr size_t kParamValueParamCountOffset = 21; // ParamValue.param_count
inline constexpr size_t kParamValueParamIndexOffset = 23; // ParamValue.param_index

// ---------------------------------------------------------------------------
// 生成式编解码：逐字段小端展开（自包含，无需手写 cpp）
// ---------------------------------------------------------------------------
namespace detail {

inline std::vector<uint8_t> makeFrame(std::span<const uint8_t> payload, uint8_t type) {
    std::vector<uint8_t> out;
    out.reserve(kFrameOverhead + payload.size());
    out.push_back(kStx0);
    out.push_back(kStx1);
    out.push_back(kProtocolVersion);
    out.push_back(static_cast<uint8_t>(payload.size()));
    out.push_back(type);
    out.insert(out.end(), payload.begin(), payload.end());
    const uint16_t crc = crc16({out.data() + 2, 3 + payload.size()});
    out.push_back(static_cast<uint8_t>(crc & 0xFF));
    out.push_back(static_cast<uint8_t>(crc >> 8));
    return out;
}

inline bool validateFrame(std::span<const uint8_t> frame, uint8_t type, size_t payload_size) {
    const size_t total = kFrameOverhead + payload_size;
    if (frame.size() != total) return false;
    if (frame[0] != kStx0 || frame[1] != kStx1) return false;
    if (frame[2] != kProtocolVersion) return false;
    if (frame[3] != payload_size) return false;
    if (frame[4] != type) return false;
    const uint16_t crc = crc16({frame.data() + 2, 3 + payload_size});
    const uint16_t got = static_cast<uint16_t>(frame[total - 2]) |
                         static_cast<uint16_t>(static_cast<uint16_t>(frame[total - 1]) << 8);
    return crc == got;
}

}  // namespace detail

// Heartbeat：编码（组完整帧）
inline std::vector<uint8_t> encodeHeartbeat(const Heartbeat& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kHeartbeatPayloadSize);
    { // mode
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.mode);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // system_type
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.system_type);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // fw_version
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.fw_version);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // system_state
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.system_state);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    return detail::makeFrame(payload, kTypeHeartbeat);
}

// Heartbeat：解码（完整帧 -> 载荷）
inline std::optional<Heartbeat> decodeHeartbeat(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeHeartbeat, kHeartbeatPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    Heartbeat out{};
    { // mode
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.mode);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // system_type
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.system_type);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 1 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // fw_version
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.fw_version);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 2 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // system_state
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.system_state);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 3 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    return out;
}

// SysStatus：编码（组完整帧）
inline std::vector<uint8_t> encodeSysStatus(const SysStatus& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kSysStatusPayloadSize);
    { // sensors_present
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.sensors_present);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // sensors_enabled
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.sensors_enabled);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // sensors_health
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.sensors_health);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // load
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.load);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // voltage_total
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.voltage_total);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // voltage_cell_max
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.voltage_cell_max);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // voltage_cell_min
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.voltage_cell_min);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // current_battery
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.current_battery);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // battery_remaining
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.battery_remaining);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // current_consumed
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.current_consumed);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // battery_temperature
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.battery_temperature);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // battery_fault_bitmask
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.battery_fault_bitmask);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // drop_rate_comm
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.drop_rate_comm);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // errors_comm
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.errors_comm);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // errors_count
        [[maybe_unused]] constexpr size_t N = 4, W = 2;
        auto* src = detail::field_ptr(p.errors_count);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // stream_mask
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.stream_mask);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeSysStatus);
}

// SysStatus：解码（完整帧 -> 载荷）
inline std::optional<SysStatus> decodeSysStatus(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeSysStatus, kSysStatusPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    SysStatus out{};
    { // sensors_present
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.sensors_present);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // sensors_enabled
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.sensors_enabled);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 4 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // sensors_health
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.sensors_health);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 8 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // load
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.load);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 12 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // voltage_total
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.voltage_total);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 14 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // voltage_cell_max
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.voltage_cell_max);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 16 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // voltage_cell_min
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.voltage_cell_min);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 18 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // current_battery
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.current_battery);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 20 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // battery_remaining
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.battery_remaining);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 22 + i * W, W);
            dst[i] = static_cast<int8_t>(raw);
        }
    }
    { // current_consumed
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.current_consumed);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 23 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // battery_temperature
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.battery_temperature);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 27 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // battery_fault_bitmask
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.battery_fault_bitmask);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 29 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // drop_rate_comm
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.drop_rate_comm);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 33 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // errors_comm
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.errors_comm);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 35 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // errors_count
        constexpr size_t N = 4, W = 2;
        auto* dst = detail::field_ptr(out.errors_count);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 37 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // stream_mask
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.stream_mask);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 45 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    return out;
}

// CommandAck：编码（组完整帧）
inline std::vector<uint8_t> encodeCommandAck(const CommandAck& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kCommandAckPayloadSize);
    { // command
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.command);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // result
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.result);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // progress
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.progress);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // result_param2
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.result_param2);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeCommandAck);
}

// CommandAck：解码（完整帧 -> 载荷）
inline std::optional<CommandAck> decodeCommandAck(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeCommandAck, kCommandAckPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    CommandAck out{};
    { // command
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.command);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // result
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.result);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 2 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // progress
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.progress);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 3 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // result_param2
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.result_param2);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 4 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    return out;
}

// PoseNed：编码（组完整帧）
inline std::vector<uint8_t> encodePoseNed(const PoseNed& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kPoseNedPayloadSize);
    { // time_boot_ms
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.time_boot_ms);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // roll
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.roll);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // pitch
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.pitch);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // yaw
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.yaw);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // rollspeed
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.rollspeed);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // pitchspeed
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.pitchspeed);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // yawspeed
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.yawspeed);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // x
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.x);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // y
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.y);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // z
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.z);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // vx
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.vx);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // vy
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.vy);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // vz
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.vz);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypePoseNed);
}

// PoseNed：解码（完整帧 -> 载荷）
inline std::optional<PoseNed> decodePoseNed(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypePoseNed, kPoseNedPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    PoseNed out{};
    { // time_boot_ms
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.time_boot_ms);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // roll
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.roll);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 4 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // pitch
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.pitch);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 8 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // yaw
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.yaw);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 12 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // rollspeed
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.rollspeed);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 16 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // pitchspeed
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.pitchspeed);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 20 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // yawspeed
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.yawspeed);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 24 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // x
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.x);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 28 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // y
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.y);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 32 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // z
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.z);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 36 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // vx
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.vx);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 40 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // vy
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.vy);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 44 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // vz
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.vz);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 48 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    return out;
}

// EkfStatusReport：编码（组完整帧）
inline std::vector<uint8_t> encodeEkfStatusReport(const EkfStatusReport& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kEkfStatusReportPayloadSize);
    { // flags
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.flags);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // velocity_variance
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.velocity_variance);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // pos_horiz_variance
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.pos_horiz_variance);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // pos_vert_variance
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.pos_vert_variance);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // compass_variance
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.compass_variance);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // terrain_alt_variance
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.terrain_alt_variance);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    return detail::makeFrame(payload, kTypeEkfStatusReport);
}

// EkfStatusReport：解码（完整帧 -> 载荷）
inline std::optional<EkfStatusReport> decodeEkfStatusReport(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeEkfStatusReport, kEkfStatusReportPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    EkfStatusReport out{};
    { // flags
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.flags);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // velocity_variance
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.velocity_variance);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 2 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // pos_horiz_variance
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.pos_horiz_variance);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 3 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // pos_vert_variance
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.pos_vert_variance);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 4 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // compass_variance
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.compass_variance);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 5 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // terrain_alt_variance
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.terrain_alt_variance);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 6 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    return out;
}

// VfrHud：编码（组完整帧）
inline std::vector<uint8_t> encodeVfrHud(const VfrHud& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kVfrHudPayloadSize);
    { // airspeed
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.airspeed);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // groundspeed
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.groundspeed);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // heading
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.heading);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // throttle
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.throttle);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // alt
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.alt);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // climb
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.climb);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeVfrHud);
}

// VfrHud：解码（完整帧 -> 载荷）
inline std::optional<VfrHud> decodeVfrHud(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeVfrHud, kVfrHudPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    VfrHud out{};
    { // airspeed
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.airspeed);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 0 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // groundspeed
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.groundspeed);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 4 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // heading
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.heading);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 8 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // throttle
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.throttle);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 10 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // alt
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.alt);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 12 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // climb
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.climb);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 16 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    return out;
}

// GlobalPositionInt：编码（组完整帧）
inline std::vector<uint8_t> encodeGlobalPositionInt(const GlobalPositionInt& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kGlobalPositionIntPayloadSize);
    { // time_boot_ms
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.time_boot_ms);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // lat
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.lat);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // lon
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.lon);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // alt
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.alt);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // relative_alt
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.relative_alt);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // vx
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.vx);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // vy
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.vy);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // vz
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.vz);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // hdg
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.hdg);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeGlobalPositionInt);
}

// GlobalPositionInt：解码（完整帧 -> 载荷）
inline std::optional<GlobalPositionInt> decodeGlobalPositionInt(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeGlobalPositionInt, kGlobalPositionIntPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    GlobalPositionInt out{};
    { // time_boot_ms
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.time_boot_ms);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // lat
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.lat);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 4 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // lon
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.lon);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 8 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // alt
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.alt);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 12 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // relative_alt
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.relative_alt);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 16 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // vx
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.vx);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 20 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // vy
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.vy);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 22 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // vz
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.vz);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 24 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // hdg
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.hdg);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 26 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    return out;
}

// GpsRawInt：编码（组完整帧）
inline std::vector<uint8_t> encodeGpsRawInt(const GpsRawInt& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kGpsRawIntPayloadSize);
    { // time_usec
        [[maybe_unused]] constexpr size_t N = 1, W = 8;
        auto* src = detail::field_ptr(p.time_usec);
        static_assert(sizeof(uint64_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint64_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // fix_type
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.fix_type);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // lat
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.lat);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // lon
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.lon);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // alt
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.alt);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // eph
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.eph);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // epv
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.epv);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // vel
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.vel);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // cog
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.cog);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // satellites_visible
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.satellites_visible);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // h_acc
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.h_acc);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // v_acc
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.v_acc);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // vel_acc
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.vel_acc);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // hdg_acc
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.hdg_acc);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeGpsRawInt);
}

// GpsRawInt：解码（完整帧 -> 载荷）
inline std::optional<GpsRawInt> decodeGpsRawInt(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeGpsRawInt, kGpsRawIntPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    GpsRawInt out{};
    { // time_usec
        constexpr size_t N = 1, W = 8;
        auto* dst = detail::field_ptr(out.time_usec);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint64_t>(raw);
        }
    }
    { // fix_type
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.fix_type);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 8 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // lat
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.lat);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 9 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // lon
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.lon);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 13 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // alt
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.alt);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 17 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // eph
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.eph);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 21 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // epv
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.epv);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 23 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // vel
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.vel);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 25 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // cog
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.cog);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 27 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // satellites_visible
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.satellites_visible);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 29 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // h_acc
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.h_acc);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 30 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // v_acc
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.v_acc);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 34 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // vel_acc
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.vel_acc);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 38 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // hdg_acc
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.hdg_acc);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 42 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    return out;
}

// WaterDepth：编码（组完整帧）
inline std::vector<uint8_t> encodeWaterDepth(const WaterDepth& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kWaterDepthPayloadSize);
    { // time_boot_ms
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.time_boot_ms);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // id
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.id);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // healthy
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.healthy);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // lat
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.lat);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // lng
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.lng);
        static_assert(sizeof(int32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // altitude
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.altitude);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // bottom_distance
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.bottom_distance);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // terrain_height
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.terrain_height);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // temperature
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.temperature);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeWaterDepth);
}

// WaterDepth：解码（完整帧 -> 载荷）
inline std::optional<WaterDepth> decodeWaterDepth(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeWaterDepth, kWaterDepthPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    WaterDepth out{};
    { // time_boot_ms
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.time_boot_ms);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // id
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.id);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 4 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // healthy
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.healthy);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 5 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // lat
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.lat);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 6 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // lng
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.lng);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 10 + i * W, W);
            dst[i] = static_cast<int32_t>(raw);
        }
    }
    { // altitude
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.altitude);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 14 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // bottom_distance
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.bottom_distance);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 18 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // terrain_height
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.terrain_height);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 22 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // temperature
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.temperature);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 26 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    return out;
}

// DistanceSensor：编码（组完整帧）
inline std::vector<uint8_t> encodeDistanceSensor(const DistanceSensor& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kDistanceSensorPayloadSize);
    { // time_boot_ms
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.time_boot_ms);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // min_distance
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.min_distance);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // max_distance
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.max_distance);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // current_distance
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.current_distance);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // type
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.type);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // id
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.id);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // orientation
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.orientation);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // covariance
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.covariance);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // horizontal_fov
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.horizontal_fov);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // vertical_fov
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.vertical_fov);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // signal_quality
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.signal_quality);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    return detail::makeFrame(payload, kTypeDistanceSensor);
}

// DistanceSensor：解码（完整帧 -> 载荷）
inline std::optional<DistanceSensor> decodeDistanceSensor(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeDistanceSensor, kDistanceSensorPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    DistanceSensor out{};
    { // time_boot_ms
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.time_boot_ms);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // min_distance
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.min_distance);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 4 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // max_distance
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.max_distance);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 6 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // current_distance
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.current_distance);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 8 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // type
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.type);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 10 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // id
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.id);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 11 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // orientation
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.orientation);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 12 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // covariance
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.covariance);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 13 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // horizontal_fov
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.horizontal_fov);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 14 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // vertical_fov
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.vertical_fov);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 18 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // signal_quality
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.signal_quality);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 22 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    return out;
}

// ManualControl：编码（组完整帧）
inline std::vector<uint8_t> encodeManualControl(const ManualControl& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kManualControlPayloadSize);
    { // sequence
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.sequence);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // x
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.x);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // y
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.y);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // z
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.z);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // p
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.p);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // r
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.r);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // yaw
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.yaw);
        static_assert(sizeof(int16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<int16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeManualControl);
}

// ManualControl：解码（完整帧 -> 载荷）
inline std::optional<ManualControl> decodeManualControl(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeManualControl, kManualControlPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    ManualControl out{};
    { // sequence
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.sequence);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // x
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.x);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 2 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // y
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.y);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 4 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // z
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.z);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 6 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // p
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.p);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 8 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // r
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.r);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 10 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    { // yaw
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.yaw);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 12 + i * W, W);
            dst[i] = static_cast<int16_t>(raw);
        }
    }
    return out;
}

// Command：编码（组完整帧）
inline std::vector<uint8_t> encodeCommand(const Command& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kCommandPayloadSize);
    { // command
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.command);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // param
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.param);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeCommand);
}

// Command：解码（完整帧 -> 载荷）
inline std::optional<Command> decodeCommand(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeCommand, kCommandPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    Command out{};
    { // command
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.command);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // param
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.param);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 2 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    return out;
}

// RcChannels：编码（组完整帧）
inline std::vector<uint8_t> encodeRcChannels(const RcChannels& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kRcChannelsPayloadSize);
    { // time_boot_ms
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.time_boot_ms);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // chancount
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.chancount);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // chan_raw
        [[maybe_unused]] constexpr size_t N = 18, W = 2;
        auto* src = detail::field_ptr(p.chan_raw);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // rssi
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.rssi);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    return detail::makeFrame(payload, kTypeRcChannels);
}

// RcChannels：解码（完整帧 -> 载荷）
inline std::optional<RcChannels> decodeRcChannels(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeRcChannels, kRcChannelsPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    RcChannels out{};
    { // time_boot_ms
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.time_boot_ms);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // chancount
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.chancount);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 4 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // chan_raw
        constexpr size_t N = 18, W = 2;
        auto* dst = detail::field_ptr(out.chan_raw);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 5 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // rssi
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.rssi);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 41 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    return out;
}

// ServoOutputRaw：编码（组完整帧）
inline std::vector<uint8_t> encodeServoOutputRaw(const ServoOutputRaw& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kServoOutputRawPayloadSize);
    { // time_boot_ms
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.time_boot_ms);
        static_assert(sizeof(uint32_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint32_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // port
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.port);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // servo_raw
        [[maybe_unused]] constexpr size_t N = 16, W = 2;
        auto* src = detail::field_ptr(p.servo_raw);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeServoOutputRaw);
}

// ServoOutputRaw：解码（完整帧 -> 载荷）
inline std::optional<ServoOutputRaw> decodeServoOutputRaw(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeServoOutputRaw, kServoOutputRawPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    ServoOutputRaw out{};
    { // time_boot_ms
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.time_boot_ms);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<uint32_t>(raw);
        }
    }
    { // port
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.port);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 4 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // servo_raw
        constexpr size_t N = 16, W = 2;
        auto* dst = detail::field_ptr(out.servo_raw);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 5 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    return out;
}

// ParamSet：编码（组完整帧）
inline std::vector<uint8_t> encodeParamSet(const ParamSet& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kParamSetPayloadSize);
    { // param_id
        [[maybe_unused]] constexpr size_t N = 16;
        auto* src = detail::field_ptr(p.param_id);
        for (size_t i = 0; i < 16; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // param_value
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.param_value);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // param_type
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.param_type);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    return detail::makeFrame(payload, kTypeParamSet);
}

// ParamSet：解码（完整帧 -> 载荷）
inline std::optional<ParamSet> decodeParamSet(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeParamSet, kParamSetPayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    ParamSet out{};
    { // param_id
        constexpr size_t N = 16, W = 1;
        auto* dst = detail::field_ptr(out.param_id);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<char>(raw);
        }
    }
    { // param_value
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.param_value);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 16 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // param_type
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.param_type);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 20 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    return out;
}

// ParamValue：编码（组完整帧）
inline std::vector<uint8_t> encodeParamValue(const ParamValue& p) {
    std::vector<uint8_t> payload;
    payload.reserve(kParamValuePayloadSize);
    { // param_id
        [[maybe_unused]] constexpr size_t N = 16;
        auto* src = detail::field_ptr(p.param_id);
        for (size_t i = 0; i < 16; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // param_value
        [[maybe_unused]] constexpr size_t N = 1, W = 4;
        auto* src = detail::field_ptr(p.param_value);
        static_assert(sizeof(float) == W);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = std::bit_cast<uint32_t>(src[i]);
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(bits >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // param_type
        [[maybe_unused]] constexpr size_t N = 1;
        auto* src = detail::field_ptr(p.param_type);
        for (size_t i = 0; i < 1; ++i) {
            payload.push_back(static_cast<uint8_t>(src[i]));
        }
    }
    { // param_count
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.param_count);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    { // param_index
        [[maybe_unused]] constexpr size_t N = 1, W = 2;
        auto* src = detail::field_ptr(p.param_index);
        static_assert(sizeof(uint16_t) == W);
        for (size_t i = 0; i < N; ++i) {
            auto v = static_cast<uint64_t>(
                static_cast<typename std::make_unsigned<uint16_t>::type>(src[i]));
            uint8_t w[W];
            for (size_t b = 0; b < W; ++b) w[b] = static_cast<uint8_t>(v >> (8 * b));
            payload.insert(payload.end(), w, w + W);
        }
    }
    return detail::makeFrame(payload, kTypeParamValue);
}

// ParamValue：解码（完整帧 -> 载荷）
inline std::optional<ParamValue> decodeParamValue(std::span<const uint8_t> frame) {
    if (!detail::validateFrame(frame, kTypeParamValue, kParamValuePayloadSize)) {
        return std::nullopt;
    }
    constexpr size_t base = 5;
    ParamValue out{};
    { // param_id
        constexpr size_t N = 16, W = 1;
        auto* dst = detail::field_ptr(out.param_id);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 0 + i * W, W);
            dst[i] = static_cast<char>(raw);
        }
    }
    { // param_value
        constexpr size_t N = 1, W = 4;
        auto* dst = detail::field_ptr(out.param_value);
        for (size_t i = 0; i < N; ++i) {
            const uint32_t bits = static_cast<uint32_t>(
                detail::getUintLE(frame, base + 16 + i * W, W));
            dst[i] = std::bit_cast<float>(bits);
        }
    }
    { // param_type
        constexpr size_t N = 1, W = 1;
        auto* dst = detail::field_ptr(out.param_type);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 20 + i * W, W);
            dst[i] = static_cast<uint8_t>(raw);
        }
    }
    { // param_count
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.param_count);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 21 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    { // param_index
        constexpr size_t N = 1, W = 2;
        auto* dst = detail::field_ptr(out.param_index);
        for (size_t i = 0; i < N; ++i) {
            const auto raw = detail::getUintLE(frame, base + 23 + i * W, W);
            dst[i] = static_cast<uint16_t>(raw);
        }
    }
    return out;
}


// ---------------------------------------------------------------------------
// 类型 -> type 字节 / 编解码函数映射（供 FrameLink::recv_frame_as 等泛型接口）
// ---------------------------------------------------------------------------
template <typename Pkt> struct packet_traits;
template <>
struct packet_traits<Heartbeat> {
    static constexpr uint8_t type = kTypeHeartbeat;
    static constexpr size_t payload_size = kHeartbeatPayloadSize;
    static constexpr auto encode = encodeHeartbeat;
    static constexpr auto decode = decodeHeartbeat;
};
template <>
struct packet_traits<SysStatus> {
    static constexpr uint8_t type = kTypeSysStatus;
    static constexpr size_t payload_size = kSysStatusPayloadSize;
    static constexpr auto encode = encodeSysStatus;
    static constexpr auto decode = decodeSysStatus;
};
template <>
struct packet_traits<CommandAck> {
    static constexpr uint8_t type = kTypeCommandAck;
    static constexpr size_t payload_size = kCommandAckPayloadSize;
    static constexpr auto encode = encodeCommandAck;
    static constexpr auto decode = decodeCommandAck;
};
template <>
struct packet_traits<PoseNed> {
    static constexpr uint8_t type = kTypePoseNed;
    static constexpr size_t payload_size = kPoseNedPayloadSize;
    static constexpr auto encode = encodePoseNed;
    static constexpr auto decode = decodePoseNed;
};
template <>
struct packet_traits<EkfStatusReport> {
    static constexpr uint8_t type = kTypeEkfStatusReport;
    static constexpr size_t payload_size = kEkfStatusReportPayloadSize;
    static constexpr auto encode = encodeEkfStatusReport;
    static constexpr auto decode = decodeEkfStatusReport;
};
template <>
struct packet_traits<VfrHud> {
    static constexpr uint8_t type = kTypeVfrHud;
    static constexpr size_t payload_size = kVfrHudPayloadSize;
    static constexpr auto encode = encodeVfrHud;
    static constexpr auto decode = decodeVfrHud;
};
template <>
struct packet_traits<GlobalPositionInt> {
    static constexpr uint8_t type = kTypeGlobalPositionInt;
    static constexpr size_t payload_size = kGlobalPositionIntPayloadSize;
    static constexpr auto encode = encodeGlobalPositionInt;
    static constexpr auto decode = decodeGlobalPositionInt;
};
template <>
struct packet_traits<GpsRawInt> {
    static constexpr uint8_t type = kTypeGpsRawInt;
    static constexpr size_t payload_size = kGpsRawIntPayloadSize;
    static constexpr auto encode = encodeGpsRawInt;
    static constexpr auto decode = decodeGpsRawInt;
};
template <>
struct packet_traits<WaterDepth> {
    static constexpr uint8_t type = kTypeWaterDepth;
    static constexpr size_t payload_size = kWaterDepthPayloadSize;
    static constexpr auto encode = encodeWaterDepth;
    static constexpr auto decode = decodeWaterDepth;
};
template <>
struct packet_traits<DistanceSensor> {
    static constexpr uint8_t type = kTypeDistanceSensor;
    static constexpr size_t payload_size = kDistanceSensorPayloadSize;
    static constexpr auto encode = encodeDistanceSensor;
    static constexpr auto decode = decodeDistanceSensor;
};
template <>
struct packet_traits<ManualControl> {
    static constexpr uint8_t type = kTypeManualControl;
    static constexpr size_t payload_size = kManualControlPayloadSize;
    static constexpr auto encode = encodeManualControl;
    static constexpr auto decode = decodeManualControl;
};
template <>
struct packet_traits<Command> {
    static constexpr uint8_t type = kTypeCommand;
    static constexpr size_t payload_size = kCommandPayloadSize;
    static constexpr auto encode = encodeCommand;
    static constexpr auto decode = decodeCommand;
};
template <>
struct packet_traits<RcChannels> {
    static constexpr uint8_t type = kTypeRcChannels;
    static constexpr size_t payload_size = kRcChannelsPayloadSize;
    static constexpr auto encode = encodeRcChannels;
    static constexpr auto decode = decodeRcChannels;
};
template <>
struct packet_traits<ServoOutputRaw> {
    static constexpr uint8_t type = kTypeServoOutputRaw;
    static constexpr size_t payload_size = kServoOutputRawPayloadSize;
    static constexpr auto encode = encodeServoOutputRaw;
    static constexpr auto decode = decodeServoOutputRaw;
};
template <>
struct packet_traits<ParamSet> {
    static constexpr uint8_t type = kTypeParamSet;
    static constexpr size_t payload_size = kParamSetPayloadSize;
    static constexpr auto encode = encodeParamSet;
    static constexpr auto decode = decodeParamSet;
};
template <>
struct packet_traits<ParamValue> {
    static constexpr uint8_t type = kTypeParamValue;
    static constexpr size_t payload_size = kParamValuePayloadSize;
    static constexpr auto encode = encodeParamValue;
    static constexpr auto decode = decodeParamValue;
};

}  // namespace ou