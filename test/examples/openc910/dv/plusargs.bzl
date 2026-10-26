"""C910-specific rv_tester plusargs, shared by every C910 test/run target."""

# rvfi_log_36b_uop=false  C910 exports a 32-bit opcode, not the internal 36-bit uop.
# rvfi_custom_uop_opcodes C910 cracks jal/jalr; register the link micro-op as a
#                         known custom op so its insn byte-check is skipped.
C910_PLUSARGS = [
    "+rvfi_log_36b_uop=false",
    "+rvfi_custom_uop_opcodes=0x0040009f:CUSTOM_MICRO_OP",
]
