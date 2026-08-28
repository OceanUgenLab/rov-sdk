## 变更摘要

<!-- 一句话说明本 PR 做了什么。 -->

## 关联 issue

<!-- 如 `Closes #12` 或 `Refs #12`。 -->

## 变更类型

- [ ] 新功能（feat）
- [ ] 缺陷修复（fix）
- [ ] 文档（docs）
- [ ] 测试 / golden（test）
- [ ] 重构（refactor）
- [ ] 其他

## 是否涉及协议变更

<!-- 若修改了 schema/protocol.yaml 或 generated/，务必勾选并说明。 -->

- [ ] 是 —— 已重新运行 `tools/codegen.py` 与 `tools/golden_gen.py`，并同步三端派生
- [ ] 否

## 测试

<!-- 说明如何验证，以及运行结果。 -->

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

- [ ] 全部测试通过

## 其他说明

<!-- 任何补充上下文、设计取舍、待办等。 -->
