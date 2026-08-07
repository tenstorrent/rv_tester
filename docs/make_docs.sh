#!/usr/bin/env bash
# SPDX-FileCopyrightText: © 2026 Tenstorrent AI ULC
# SPDX-License-Identifier: Apache-2.0
#
# One-command documentation build.
#
# Finds a suitable Python, creates a private virtualenv with the doc
# dependencies (only the first time), then builds the Sphinx docs. Any
# arguments are passed straight through to docs/build.py, e.g.:
#
#   ./docs/make_docs.sh                 # build into docs/_build
#   ./docs/make_docs.sh --local_host    # build and serve locally
#   ./docs/make_docs.sh --check         # strict build (warnings = errors)
#
set -euo pipefail

DOCS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="$DOCS_DIR/.venv"
# Dependencies live in docs/requirements.txt so this script and CI install the
# same pinned set. The stamp is the file's contents, so editing requirements.txt
# automatically forces a reinstall on the next run.
REQS_FILE="$DOCS_DIR/requirements.txt"
STAMP_EXPECTED="$(cat "$REQS_FILE")"

# --- 1. Find a Python new enough to run Sphinx (needs >= 3.9) ----------------
find_python() {
    for py in python3.13 python3.12 python3.11 python3.10 python3.9 python3 python; do
        if command -v "$py" >/dev/null 2>&1; then
            if "$py" -c 'import sys; sys.exit(0 if sys.version_info[:2] >= (3, 9) else 1)' 2>/dev/null; then
                echo "$py"
                return 0
            fi
        fi
    done
    return 1
}

# --- 2. Create / refresh the virtualenv --------------------------------------
if [ ! -x "$VENV_DIR/bin/sphinx-build" ] || [ "$(cat "$VENV_DIR/.reqs_stamp" 2>/dev/null || true)" != "$STAMP_EXPECTED" ]; then
    PYTHON="$(find_python)" || {
        echo "ERROR: need Python >= 3.9 to build the docs, but none was found." >&2
        echo "       Install a modern Python (e.g. python3.12) and re-run." >&2
        exit 1
    }
    echo "Setting up docs virtualenv at $VENV_DIR using $PYTHON ..."
    "$PYTHON" -m venv "$VENV_DIR"
    "$VENV_DIR/bin/pip" install --quiet --upgrade pip
    "$VENV_DIR/bin/pip" install --quiet -r "$REQS_FILE"
    echo "$STAMP_EXPECTED" > "$VENV_DIR/.reqs_stamp"
    echo "Done. (This one-time setup is cached; future runs are fast.)"
fi

# --- 3. Build (sphinx-build must be on PATH; build.py shells out to it) -------
export PATH="$VENV_DIR/bin:$PATH"
exec "$VENV_DIR/bin/python" "$DOCS_DIR/build.py" "$@"
