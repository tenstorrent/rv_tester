// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <cstdint>

namespace whisper {
void initialize();
std::string disassemble(const uint32_t opcode);
}; // namespace whisper
