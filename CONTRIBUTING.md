# Contributing

Thanks for contributing to ou_sdk. This repository owns the **implementation and docs of the communication protocol** shared by the three ends of the underwater robot (host-station SDK, compute board, STM32 firmware), so protocol changes must follow the single path below.

## The only path for protocol changes

The protocol uses `schema/protocol.yaml` as its **single hand-written source of truth**. Every artifact under `generated/` is a derived result and **must not be edited by hand**. Any protocol change must go through these steps in order:

```
Edit schema/protocol.yaml
        │
        ▼
python3 tools/codegen.py          # regenerate C++ header / C header / protocol docs / ROS2 .msg
        │
        ▼
python3 tools/golden_gen.py       # regenerate golden byte vectors
        │
        ▼
Sync derivation to all three ends # C++ header (this repo) / C header (rov-firmware) / ROS2 msg (ros2 repo)
        │
        ▼
Regression tests + byte-for-byte golden comparison
```

- To change the protocol, **edit only the schema**. Then run codegen + golden_gen and use `git diff` to confirm that the changes under `generated/` match the field changes you intended.
- golden is the test anchor: `tests/test_protocol.cpp` includes `generated/golden/golden.h` and compares byte for byte, so all three implementations must encode/decode identical bytes.
- A breaking change (modifying the frame header or payload layout) must bump the `version` in `schema/protocol.yaml` and be recorded as `breaking change` in `CHANGELOG.md`.

## Commit convention

Follow [Conventional Commits](https://www.conventionalcommits.org/). Commit messages are always in English:

```
<type>(<scope>): <subject>
```

| type | Purpose |
|------|---------|
| `feat` | New feature / module |
| `fix` | Bug fix |
| `docs` | Documentation |
| `test` | Tests and golden vectors |
| `refactor` | Refactor (no behavior change) |
| `build` | Build / toolchain / dependencies |
| `ci` | CI/CD pipeline |
| `chore` | Misc (cleanup, retire) |

- **scope**: `schema` `codegen` `proto` `channel` `link` `golden` `oss` (omit scope for cross-module changes)
- **subject**: imperative mood, lowercase first letter, starts with a verb (`add`/`fix`/`remove`/`refactor`/`update`/`bump`), ≤50 characters, no trailing period

## Branches and PRs

Branches follow Git Flow: `main` (stable releases, tagged) + `develop` (integration) + `feature/<type>/<kebab-case>` (branched from develop, deleted after merge) + `release/vX.Y.Z` + `hotfix/<topic>`.

- Merge feature → develop with a **squash merge**, require ≥1 approval (the author cannot self-approve), and merge only when CI is fully green.
- Run `git rebase develop` before opening a PR (resolve conflicts via rebase). Use `<type>(<scope>): <subject>` for the PR title and the shared template for the PR body (background / changes / verification / screenshots).
- main/develop: no force push, no deletion, and Require PR enabled.
- Full specification: the company Git standards knowledge base at https://ccnl4e0p1x4e.feishu.cn/docx/JbFHdcashoNTL6x34YxcaGyAnEg

## Testing conventions

- Tests live in `tests/test_*.cpp`. Each file is a **standalone executable** (with its own `main()` and assertion macros) and is registered with `ctest` one by one by `tests/CMakeLists.txt`.
- All tests must pass before you commit:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

- When adding a frame type or changing a field, always update the golden vectors and the corresponding tests to preserve the red-green cycle (fail first, then pass).

## Code style

- C++20, standard library + native POSIX APIs, no external dependencies.
- All comments and docs in English; identifiers and namespaces in English (`ou::`).
- Follow the minimal-change principle: touch only files directly related to the current task and do not refactor unrelated code along the way.
