# SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

# Checks the format string of every cvm::log(cvm::ERROR, ...) call.
# The logger prepends +logger_error_prefix ("Error: ") to cvm::ERROR lines and
# triage scripts read "<prefix><message>" from a single line, so a message must:
#   - not start with "\n" (it would split the prefix from the message)
#   - not hard-code its own "Error:" / "ERROR:" prefix (it would be doubled)
#   - end with "\n"
# Calls whose format argument is not a string literal are skipped.

import os
import re
import sys


IGNORE_DIRECTORIES = ["./.git", "./infra", "./build", "./regressions"]
SOURCE_SUFFIXES = (".c", ".cc", ".cpp", ".h", ".hpp")

CALL_RE = re.compile(r"cvm::ERROR\s*,")
LITERAL_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')
ERROR_PREFIX_RE = re.compile(r"^\s*\[?\s*error\s*\]?\s*[:\-]", re.IGNORECASE)


def scan_directory(directory, source_files):
    for entry in os.scandir(directory):
        path = os.fsdecode(entry.path)
        if path in IGNORE_DIRECTORIES:
            continue
        if entry.is_dir(follow_symlinks=False):
            scan_directory(path, source_files)
        elif path.endswith(SOURCE_SUFFIXES):
            source_files.append(path)


# Returns the text of the call argument starting at `start`, up to the next
# top-level ',' or ')'.
def argument_at(src, start):
    depth = 0
    i = start
    while i < len(src):
        c = src[i]
        if c in "\"'":
            i += 1
            while i < len(src) and src[i] != c:
                i += 2 if src[i] == "\\" else 1
        elif c in "([{":
            depth += 1
        elif c in ")]}":
            if depth == 0:
                break
            depth -= 1
        elif c == "," and depth == 0:
            break
        i += 1
    return src[start:i].strip()


def check_file(path):
    with open(path, "r", errors="replace") as f:
        src = f.read()

    problems = []
    for m in CALL_RE.finditer(src):
        arg = argument_at(src, m.end())
        literals = LITERAL_RE.findall(arg)
        if not literals or not arg.startswith('"') or not arg.endswith('"'):
            continue

        line = src.count("\n", 0, m.start()) + 1
        if literals[0].startswith("\\n"):
            problems.append((line, 'starts with "\\n"'))
        if ERROR_PREFIX_RE.match(literals[0]):
            problems.append((line, "hard-codes an error prefix; the logger adds +logger_error_prefix"))
        if not literals[-1].endswith("\\n"):
            problems.append((line, 'does not end with "\\n"'))

    return ["{}:{}: cvm::ERROR message {}".format(path, line, why) for line, why in problems]


def main():
    source_files = []
    scan_directory(".", source_files)

    errors = []
    for path in sorted(source_files):
        errors.extend(check_file(path))

    for e in errors:
        print(e)
    sys.exit(1 if errors else 0)


if __name__ == "__main__":
    main()
