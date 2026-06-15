#include "whisper_decoder.h"

#include "DecodedInst.hpp"
#include "Decoder.hpp"
#include "Disassembler.hpp"

static WdRiscv::Decoder decoder;
static WdRiscv::Disassembler disassembler;

void whisper::initialize() {
  decoder.enableRv64(true);
}

std::string whisper::disassemble(const uint32_t opcode) {
  std::string disas;
  disassembler.disassembleInst(opcode, decoder, disas);
  return disas;
}
