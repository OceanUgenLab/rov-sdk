# OU 通信协议 (v1)

> 本仓库为协议实现与文档的持有方。三端（上位机 SDK、算力板、STM32 固件）均以本文档为唯一权威源实现，任何修改必须同步更新 golden 帧向量（`tests/test_protocol.cpp`）并回归测试。

## 1. 帧格式

```
AA 55 | len(1B) | type(1B) | payload(len) | crc16(2B, 小端)
```

| 字段 | 长度 | 说明 |
|------|------|------|
| STX0 | 1B | 固定 `0xAA` |
| STX1 | 1B | 固定 `0x55` |
| len | 1B | payload 字节数（不含帧头、不含 CRC） |
| type | 1B | 帧类型：`0x01` 控制指令 / `0x02` 遥测 |
| payload | len | 载荷（见下） |
| crc16 | 2B | CRC-16/MODBUS，小端；计算范围 = `len` 起（不含 STX），长度 `2 + len` |

- 无 ETX 结束符。
- 帧总长 = `6 + len`。
- **字节序固定小端**（与 STM32 一致），浮点按 IEEE-754 位模式传输。

## 2. CRC-16/MODBUS

- init `0xFFFF`，poly `0xA001`，反射输入/输出，无 xorout。
- 标准校验向量：`crc16("123456789") == 0x4B37`。
- 覆盖范围：`len`、`type`、`payload` 三字节段（从 `len` 起算，与 `proto_crc16(&out[2], 2+len)` 一致）。

## 3. 载荷定义

### 3.1 控制指令 `type=0x01`（主控 → 从控），payload 26 字节

| 偏移 | 字段 | 类型 | 说明 |
|------|------|------|------|
| 0 | surge | float32 | 前(+) / 后(-)，[-1, 1] |
| 4 | sway | float32 | 右平移(+) / 左平移(-)，[-1, 1] |
| 8 | heave | float32 | 上浮(+) / 下潜(-)，[-1, 1] |
| 12 | yaw | float32 | 右转(+) / 左转(-)，[-1, 1] |
| 16 | servo | float32 | 舵机角度 0..180 度 |
| 20 | arm | float32 | 机械臂 0..100 % |
| 24 | light[0] | uint8 | 照明灯 1，0..100 % |
| 25 | light[1] | uint8 | 照明灯 2，0..100 % |

### 3.2 遥测 `type=0x02`（从控 → 主控），payload 52 字节

| 偏移 | 字段 | 类型 | 说明 |
|------|------|------|------|
| 0 | depth | float32 | 水深 m |
| 4 | heading | float32 | 航向 deg |
| 8 | roll | float32 | 横滚 deg |
| 12 | pitch | float32 | 俯仰 deg |
| 16 | battery | float32 | 电池电压 V |
| 20 | thr[0..7] | float32×8 | 各推进器指令百分比 [-1, 1] |

## 4. Golden 帧示例

以下字节序列是协议实现的测试锚点，三端实现必须编码/解码出完全一致的字节。

### 4.1 控制指令帧（32 字节）

载荷：surge=0.5, sway=-0.25, heave=1.0, yaw=0.125, servo=90.0, arm=50.0, light=[100,0]

```
AA 55 1A 01
00 00 00 3F    surge=0.5f    (0x3F000000 LE)
00 00 80 BE    sway=-0.25f   (0xBE800000 LE)
00 00 80 3F    heave=1.0f    (0x3F800000 LE)
00 00 00 3E    yaw=0.125f    (0x3E000000 LE)
00 00 B4 42    servo=90.0f   (0x42B40000 LE)
00 00 48 42    arm=50.0f     (0x42480000 LE)
64 00          light=[100,0]
XX XX          crc16 (len..payload, 小端)
```

### 4.2 遥测帧（58 字节）

载荷：depth=12.5, heading=180.0, roll=-3.0, pitch=2.5, battery=25.2, thr[8]=[-0.4,-0.3,-0.2,-0.1,0.0,0.1,0.2,0.3]

```
AA 55 34 02
00 00 48 41    depth=12.5f    (0x41480000 LE)
...            (其余字段按上述规则编码)
XX XX          crc16
```

## 5. 实现约束

- 解析必须**滑动窗口**处理流（容忍噪声前缀/粘包），STX 不匹配时逐字节推进。
- len/type/CRC 任一校验失败即丢弃该帧（跳到下一帧边界）。
- 载荷校验失败时**不修改**输出缓冲区。
- 三端实现以 `tests/test_protocol.cpp` 的 golden 向量为验收标准。

## 6. 变更流程

1. 修改本文档 + `tests/test_protocol.cpp`（新增/更新 golden 向量）。
2. 三端实现同步，各自通过 golden 测试。
3. 汇总文档至 master 仓库。
