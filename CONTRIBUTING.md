# Contributing to rv_tester

Thank you for your interest in contributing to rv_tester! This document
describes how to build the project, the standards we expect of contributions,
and the process for submitting changes.

By participating in this project, you agree to abide by our
[Code of Conduct](CODE_OF_CONDUCT.md).

## Reporting Issues

- **Bugs and feature requests** are tracked via [GitHub Issues](https://github.com/tenstorrent/rv_tester/issues). Please include enough detail to reproduce a bug or understand a request.
- **Security vulnerabilities** must **not** be filed as public issues — follow the process in [SECURITY.md](SECURITY.md).

## Getting Started

rv_tester builds with [Bazel](https://bazel.build/) (bzlmod).

```sh
# Build the exported targets
bazel build //...

# Run the tests
bazel test //test/...
```

## Coding Standards

- **Formatting** — C++ is formatted with `clang-format` using the repo's `.clang-format`; `clang-tidy` checks are configured in `.clang-tidy`.
- **Tests** — add or update tests under `test/` for any behavior change.
- **SPDX headers** — every source file must carry an SPDX header. This repository is [REUSE](https://reuse.software) compliant and CI enforces it with `reuse lint`. Use:

  ```cpp
  // SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
  // SPDX-License-Identifier: Apache-2.0
  ```

  Files that cannot carry a comment header are covered by `REUSE.toml`.

## Submitting Changes

Bug fixes and new functionality are submitted via **Pull Requests**:

1. Fork the repository and create a topic branch from the default branch.
2. Make your changes, keeping commits focused with clear messages.
3. Ensure the build, tests, and `reuse lint` all pass.
4. Open a pull request describing the motivation and the change, and link any related issue.

Pull requests are reviewed on a **weekly** cadence. Please respond to review
feedback and keep your branch up to date with the default branch.
