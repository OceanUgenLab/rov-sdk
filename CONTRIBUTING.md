# 贡献指南

感谢你为 ou_sdk 贡献代码。本仓库是水下机器人三端（上位机 SDK、算力板、STM32 固件）**通信协议的实现与文档持有方**，协议变更必须遵循唯一路径。

## 修改协议的唯一路径

协议以 `schema/protocol.yaml` 为**唯一手写真源**，`generated/` 下所有产物均为派生结果，**禁止手改**。任何协议变更必须依次走完：

```
改 schema/protocol.yaml
        │
        ▼
python3 tools/codegen.py          # 重新生成 C++ 头 / C 头 / 协议文档 / ROS2 .msg
        │
        ▼
python3 tools/golden_gen.py       # 重新生成 golden 字节向量
        │
        ▼
三端派生同步                      # C++ 头(本仓库) / C 头(rov-firmware) / ROS2 msg(ros2 仓库)
        │
        ▼
回归测试 + 逐字节 golden 比对
```

- 改协议**只改 schema**，改完跑 codegen + golden_gen，`git diff` 确认 `generated/` 的差异与你预期的字段变更一致。
- golden 是测试锚点：`tests/test_protocol.cpp` 引用 `generated/golden/golden.h` 逐字节比对，三端实现必须编码/解码出完全一致的字节。
- 破坏性变更（改帧头、改载荷布局）必须提升 `schema/protocol.yaml` 的 `version`，并在 `CHANGELOG.md` 记录为 `破坏性变更`。

## 提交规范

遵循 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/)。提交信息用中文描述，类型前缀用英文：

```
<type>(<scope>): <中文摘要>
```

| type | 用途 |
|------|------|
| `feat` | 新功能 / 新模块 |
| `fix` | 缺陷修复 |
| `docs` | 文档变更 |
| `test` | 测试与 golden 变更 |
| `refactor` | 重构（不改行为） |
| `chore` | 构建/工具/CI 等杂项 |

scope 示例：`schema` / `codegen` / `proto` / `channel` / `link` / `oss`。

## 测试约定

- 测试位于 `tests/test_*.cpp`，每个文件是**独立可执行程序**（自带 `main()` 与断言宏），由 `tests/CMakeLists.txt` 逐个接入 `ctest`。
- 提交前必须保证全部测试通过：

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

- 新增帧类型 / 改字段，务必同步更新 golden 向量与对应测试，保证「先失败后通过」的红绿循环。

## 代码风格

- C++20，标准库 + POSIX 原生 API，无外部依赖。
- 所有注释、文档用中文；标识符、命名空间用英文（`ou::`）。
- 遵循「最小改动」原则：只改与本次任务直接相关的文件，不顺手重构无关代码。
