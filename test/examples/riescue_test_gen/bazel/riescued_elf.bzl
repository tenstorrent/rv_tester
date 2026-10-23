"""Macro generating a test ELF with riescued, so every test shares the same
toolchain flags and build-time whisper validation."""

def riescued_elf(name, testfile, visibility = None):
    """Generate <name>.elf (+ .dis, _whisper.log) from RiescueD source
    `testfile`, whose basename must be "<name>.s" (riescued names outputs
    after the test file's stem). Fixed seed keeps the output reproducible;
    --run_iss makes a broken ELF fail the build, not the cosim.
    """
    if not testfile.endswith("/" + name + ".s") and testfile != name + ".s":
        fail("riescued_elf: testfile basename must be '{}.s', got '{}'".format(name, testfile))
    native.genrule(
        name = name + "_elf",
        srcs = [
            testfile,
            "//test/examples/riescue_test_gen:config/rv_tester_cpu_config.json",
        ],
        outs = [
            name + ".elf",
            name + ".dis",
            name + "_whisper.log",
        ],
        cmd = """
            run_dir=$(RULEDIR)/{name}.run
            rm -rf $$run_dir && mkdir -p $$run_dir
            $(location //test/examples/riescue_test_gen:riescued) \\
                --testfile $(location {testfile}) \\
                --cpuconfig $(location //test/examples/riescue_test_gen:config/rv_tester_cpu_config.json) \\
                --seed 1 \\
                --run_dir $$run_dir \\
                --compiler_march rv64imac_zicsr_zicntr_zifencei \\
                --compiler_opts=-mabi=lp64 \\
                --no_random_csr_reads \\
                --compiler_path $(location @riscv_none_elf_gcc//:gcc) \\
                --disassembler_path $(location @riscv_none_elf_gcc//:objdump) \\
                --whisper_path $(location @whisper//:whisper) \\
                --run_iss
            cp $$run_dir/{name} $(location {name}.elf)
            cp $$run_dir/{name}.dis $(location {name}.dis)
            cp $$run_dir/{name}_whisper.log $(location {name}_whisper.log)
        """.format(name = name, testfile = testfile),
        tools = [
            "//test/examples/riescue_test_gen:riescued",
            "@riscv_none_elf_gcc//:all_files",
            "@riscv_none_elf_gcc//:gcc",
            "@riscv_none_elf_gcc//:objdump",
            "@whisper//:whisper",
        ],
        visibility = visibility,
    )
