# ou_sdk — 水下机器人上位机侧 SDK

C++20 实现的水下机器人上位机侧 SDK。当前包含**通信协议编解码**（二进制帧协议，三端共享契约），规划扩展数据模型与传输层。

## 构建

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ctest
```

## 协议

帧格式 `AA 55 | len | type | payload | crc16` 的完整定义与 golden 帧向量见 [`docs/protocol.md`](docs/protocol.md)。协议为三端（上位机 SDK / 算力板 / STM32 固件）公共接口，本仓库为实现与文档持有方。

## 集成

```cmake
add_subdirectory(ou_sdk)   # 或 find_package
target_link_libraries(app PRIVATE ou::proto)
```

## 许可

公司内部代码仓库，仅限内部授权人员使用。
