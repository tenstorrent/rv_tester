# SPDX-FileCopyrightText: © 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0
"""Bazel entry point for RiescueD (@riescue is a plain py_library)."""

from riescue.riescued import main

if __name__ == "__main__":
    main()
