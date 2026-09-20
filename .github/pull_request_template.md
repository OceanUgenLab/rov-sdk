## Summary

<!-- Describe in one sentence what this PR does. -->

## Related issue

<!-- For example `Closes #12` or `Refs #12`. -->

## Type of change

- [ ] New feature (feat)
- [ ] Bug fix (fix)
- [ ] Docs (docs)
- [ ] Tests / golden (test)
- [ ] Refactor (refactor)
- [ ] Other

## Does this involve a protocol change?

<!-- If you modified schema/protocol.yaml or generated/, check this and explain. -->

- [ ] Yes: re-ran `tools/codegen.py` and `tools/golden_gen.py` and synced the derivation across all three ends
- [ ] No

## Tests

<!-- Describe how to verify and the results. -->

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

- [ ] All tests pass

## Additional notes

<!-- Any additional context, design tradeoffs, TODOs, etc. -->
