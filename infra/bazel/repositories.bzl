load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def risc_p_cores_dependencies():

    whisper_hash="0d1b9ad70f82c0e6501d8d7757e18b9931ece3b2"
    maybe(
        git_repository,
        name = "whisper",
        commit = whisper_hash,
        shallow_since = "1656867071 -0400",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/swerv-iss.git",
    )

    maybe(
        http_archive,
        name = "rules_foreign_cc",
        sha256 = "960e4a925dee4104073d80ef1a8e206b4c241b3f771301b9f57932d6b01529fb",
        strip_prefix = "bazelbuild-rules_foreign_cc-0.8.0",
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/bazelbuild-rules_foreign_cc/-/archive/0.8.0/bazelbuild-rules_foreign_cc-0.8.0.tar.gz",
    )
    
    maybe(
        git_repository,
        name = "riscv-dv",
        commit = "2fe4bccf28d735134d9f3e2680d806b4e41f8a90",
        shallow_since = "1666827659 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/riscv-dv.git",
    )

    maybe(
        git_repository,
        name = "rv_sim",
        commit = "d1e2d79f136e2256c526b30577d5b2f90e76937e",
        shallow_since = "1646793872 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/rv_sim.git",
    )

    maybe(
        http_archive,
        name = "uvm",
        sha256 = "502a2e605ce552bfd9767803c7e99a053715b00f7a9c4c511c3fbfddfb30157c",
        url = "https://www.accellera.org/images/downloads/standards/uvm/uvm-1.2.tar.gz",
        strip_prefix = "uvm-1.2",
        build_file_content = """
filegroup(
    name = "uvm",
    srcs = glob(["src/**"]),
    visibility = ["//visibility:public"],
)
        """
    )

    maybe(
        http_archive,
        name         = "com_github_gflags_gflags",
        url          = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/gflags/-/archive/v2.2.2/gflags-v2.2.2.tar.gz",
        sha256       = "f9859e5026509ef7db263166ad739db8b8ad6ec34ce3faecc2619d1f56ff25fb",
        strip_prefix = "gflags-v2.2.2",
    )

    gen_csr_hash="c69c68f0e574a173afa838209a4005de00c38d7e"
    maybe(
        http_archive,
        name = "gen-csr",
        strip_prefix = "gen-csr-{commit}".format(commit=gen_csr_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/gen-csr/-/archive/{commit}/gen-csr-{commit}.tar.bz2".format(commit=gen_csr_hash),
    )

    core_arch_checker_hash="b944577c128e2c09dda8db43555a84f599280a82"
    maybe(
        http_archive,
        name = "CoreArchChecker",
        strip_prefix = "CoreArchChecker-{commit}".format(commit=core_arch_checker_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/CoreArchChecker/-/archive/{commit}/CoreArchChecker-{commit}.tar.bz2".format(commit=core_arch_checker_hash),
    )

    testgen_hash="9d2159d4f7588b16285964407c327542bc3dfde4"
    maybe(
        git_repository,
        name = "testgen",
        commit = testgen_hash,
        shallow_since = "1677278961 -0600",
        recursive_init_submodules = True,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/testgen.git",
    )

    verif_lib_hash="702058648c3f8d540bb9c5db09949149be11c365"
    maybe(
        git_repository,
        name = "verif_lib",
        commit = verif_lib_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/verif_lib.git",
    )

    sharedcache_hash="1a3610af0fbb4e0dcb735aa51ff0c0cc71a85340"
    maybe(
        git_repository,
        name = "sharedcache",
        commit = sharedcache_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/sharedcache.git",
    )

    MemCache_hash="f2ac2306ec6d337f6af5cc2361a520f48a84990f"
    maybe(
        git_repository,
        name = "MemCache",
        commit = MemCache_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/MemCache.git",
    )

    arteris_ncore_fabric_hash="a70f46820c231f3b572b2fe941be4c46c03c8364"
    maybe(
        git_repository,
        name = "arteris_ncore_fabric",
        commit = arteris_ncore_fabric_hash,
        shallow_since = "1667196368 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/vendor/arteris_ncore.git",
    )

    rv_common_hash="d9cb4060c33854d31f0708230df0b0fe5332c09d"
    maybe(
        git_repository,
        name = "rv-common",
        commit = rv_common_hash,
        shallow_since = "1658763133 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/rv-common.git",
    )

    rules_python_version = "0.11.0"
    maybe(
        http_archive,
        name = "rules_python",
        sha256 = "e174d5a09fbca6f292a97f64e2f95e46ef6c4b33d7917269db81901ed439c3bb",
        strip_prefix = "rules_python-{}".format(rules_python_version),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/rules_python/-/archive/0.11.0/rules_python-{}.zip".format(rules_python_version),
    )

    maybe(
        http_archive,
        name = "rapid_json",
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/rapidjson/-/archive/v1.1.0/rapidjson-v1.1.0.tar.gz",
        sha256 = "b36fab20fa505b9baa4c51ba1e43817c87ad46fa63931a3063d8d1d77a224269",
        strip_prefix = "rapidjson-v1.1.0",
        build_file_content = """
cc_library(
    name = "rapid_json",
    hdrs = glob(["include/**"]),
    includes = ["include"],
    strip_include_prefix = "include/",
    visibility = ["//visibility:public"],
)

    """
    )

    SQLITE_BAZEL_COMMIT="428bae7fc6af34424e2543cafb767fae8fa8d588"
    maybe(
        http_archive,
        name = "com_github_rockwotj_sqlite_bazel",
        strip_prefix = "sqlite-bazel-" + SQLITE_BAZEL_COMMIT,
        urls = ["https://aus-gitlab.local.tenstorrent.com/riscv/forks/sqlite-bazel/-/archive/428bae7fc6af34424e2543cafb767fae8fa8d588/sqlite-bazel-%s.zip" % SQLITE_BAZEL_COMMIT],
    build_file_content = """
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "sqlite3",
    srcs = ["sqlite3.c"],
    hdrs = ["sqlite3.h"],
    strip_include_prefix = ".",
    visibility = ["//visibility:public"],
    deps = [],
)
""",
    )

    maybe(
        http_archive,
        name = "yaml-cpp",
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/yaml-cpp/-/archive/yaml-cpp-0.7.0/yaml-cpp-yaml-cpp-0.7.0.tar.gz",
        sha256 = "43e6a9fcb146ad871515f0d0873947e5d497a1c9c60c58cb102a97b47208b7c3",
        strip_prefix = "yaml-cpp-yaml-cpp-0.7.0",
    )

    maybe(
        http_archive,
        name = "gperftools_com",
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/gperftools/-/archive/gperftools-2.10/gperftools-gperftools-2.10.tar.gz",
        sha256 = "b0dcfe3aca1a8355955f4b415ede43530e3bb91953b6ffdd75c45891070fe0f1",
        strip_prefix = "gperftools-gperftools-2.10",
        build_file_content = """
cc_library(
    name = "gperftools_com",
    hdrs = glob(["src/gperftools/malloc_extension.h"]),
    strip_include_prefix = "src/",
    visibility = ["//visibility:public"],
)

    """
    )

    grendel_perf_hash = "4b06a78557d63c10029dcd770affbfe991b19f4c"
    maybe(
        new_git_repository,
        name = "grendel-perf",
        remote = "git@aus-gitlab.local.tenstorrent.com:arch/grendel-perf.git",
        commit = grendel_perf_hash,
        recursive_init_submodules = True,
        build_file_content = """
cc_library(
    name = "bp_model",
    hdrs = glob(["models/cpu/bp/as_tage/src/*.hpp",
                 "models/cpu/bp/nfp/src/*.hpp"]),
    srcs = glob([
                 "models/cpu/bp/as_tage/src/*.cpp", 
                 "models/cpu/bp/nfp/src/*.cpp", 
                ]),
    deps = [
            "@grendel-perf//:bp_common",
            "@grendel-perf//:sparta_com",
            "@grendel-perf//:cache_preload",
            "@grendel-perf//:cbp",
            "@grendel-perf//:cpu_common",
            ],
    visibility = ["//visibility:public"],
    linkopts = ["-lboost_filesystem", "-lboost_serialization", "-lboost_timer"],
)
cc_library(
    name = "bp_common",
    hdrs = glob(["models/cpu/bp/common/*.hpp"]),
    visibility = ["//visibility:public"],
    strip_include_prefix = "models/",
)
cc_library(
    name = "cbp",
    hdrs = glob(["models/cpu/bp/cbp/src/*.h"]),
    visibility = ["//visibility:public"],
    strip_include_prefix = "models/",
)
cc_library(
    name = "cpu_common",
    hdrs = glob([
                 "models/cpu/common/Logging.hpp",
                 "models/cpu/common/Types.hpp",
                 "models/cpu/common/Trace.hpp",
                 "models/cpu/common/LRU.hpp",
                 "models/cpu/common/Utils.hpp",
                 ]),
    deps = [
            ":other",
            ":cpu_src",
            ":cache_repl",
            ],
    visibility = ["//visibility:public"],
    strip_include_prefix = "models/",
)
cc_library(
    name = "other",
    hdrs = glob(["models/cpu/common/BaseTypes.hpp"]),
    visibility = ["//visibility:public"],
    strip_include_prefix = "models/cpu/common/",
)
cc_library(
    name = "cpu_src",
    hdrs = glob(["models/cpu/src/TraceReader.hpp"]),
    srcs = glob(["models/cpu/src/TraceReader.cpp"]),
    visibility = ["//visibility:public"],
    strip_include_prefix = "models/cpu/src/",
)
cc_library(
    name = "cache_repl",
    hdrs = (["map/sparta/cache/ReplacementIF.hpp"]),
    visibility = ["//visibility:public"],
    strip_include_prefix = "map/sparta/",
)
cc_library(
    name = "sparta_com",
    hdrs = glob(["map/sparta/sparta/**/*.hpp" , "map/sparta/src/State.tpp"]),
    srcs = glob(["map/sparta/src/*.cpp"]),
    deps = [
            "@rapid_json//:rapid_json",
            "@grendel-perf//:simdb_com",
            "@yaml-cpp//:yaml-cpp",
            "@gperftools_com//:gperftools_com",
            ],
    visibility = ["//visibility:public"],
    strip_include_prefix = "map/sparta/",
    linkopts = ["-lhdf5"],
)
cc_library(
    name = "cache_preload",
    hdrs = glob(["map/sparta/cache/preload/*.hpp"]),
    visibility = ["//visibility:public"],
    strip_include_prefix = "map/sparta/",
)
cc_library(
    name = "simdb_com",
    hdrs = glob(["map/sparta/simdb/include/simdb/**/*.hpp", "map/sparta/simdb/include/*.hpp"]),
    srcs = glob(["map/sparta/simdb/src/*.cpp"]),
    deps = ["@com_github_rockwotj_sqlite_bazel//:sqlite3"],
    visibility = ["//visibility:public"],
    strip_include_prefix = "map/sparta/simdb/include/",
)
""")

    rules_chisel_hash="df6e23db3283f3df610d425418b421309ba7698b"
    maybe(
        http_archive,
        name = "rules_chisel",
        sha256 = "7c805318afd63e008501d48df541e356380bacb126745771215da3a044506113",
        strip_prefix = "rules_chisel-{commit}".format(commit=rules_chisel_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/bazel/rules_chisel/-/archive/{commit}/rules_chisel-{commit}.tar.bz2".format(commit=rules_chisel_hash),
    )

    chisel_math_hash="70129c55dc1c2dc436a357b590c38930631638dc"
    maybe(
        git_repository,
        name = "chisel-math",
        commit = chisel_math_hash,
        shallow_since = "1664916258 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/rtl/chisel-math.git",
    )

    bazel_pysv_hash="a167f6028fbb793ee637e07a15d2bf43bb8c64c8"
    maybe(
        git_repository,
        name = "bazel_pysv",
        commit = bazel_pysv_hash,
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/bazel_pysv.git",
    )

    maybe(
        http_archive,
        name = "csv-parser",
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/forks/vincentlaucsb-csv-parser/-/archive/2.1.3/vincentlaucsb-csv-parser-2.1.3.tar.gz",
        sha256 = "5d85423dd24f609cb9c1c366ba9844d21eb9c7aaf27b1cf5e824bf3c6a723597",
        strip_prefix = "vincentlaucsb-csv-parser-2.1.3",
        build_file_content = """
cc_library(
    name = "csv-parser",
    hdrs = glob(["single_include/csv.hpp"]),
    strip_include_prefix = "single_include/",
    linkopts = ["-lpthread"],
    visibility = ["//visibility:public"],
)

    """
    )

    vec_specs_hash="7169f87fbfdb75fd7f9dff633d714726a3dcb66a"
    maybe(
        git_repository,
        name = "vec_specs",
        commit = vec_specs_hash,
        shallow_since = "1664916258 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/vec_specs.git",
    )

    rules_vcf_hash = "dbf53726370917c8229277ca828438ad10c8b23b"
    maybe(
        git_repository,
        name = "rules_vcf",
        commit = rules_vcf_hash,
        shallow_since = "1664916258 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/bazel/rules_vcf.git",
    )

    rv_tester_hash="969de823418cd8045c888da8e439a23a3ef6e990"
    maybe(
        http_archive,
        name = "rv_tester",
        strip_prefix = "rv_tester-{commit}".format(commit=rv_tester_hash),
        url = "https://aus-gitlab.local.tenstorrent.com/riscv/dv/rv_tester/-/archive/{commit}/rv_tester-{commit}.tar.bz2".format(commit=rv_tester_hash),
    )

    vector_crack_wrapper_hash="95cf8dfe67200c65e0fffadda3e58035261c4c24"
    maybe(
        git_repository,
        name = "vector-crack-wrapper",
        commit = vector_crack_wrapper_hash,
        shallow_since = "1664916259 -0500",
        remote = "git@aus-gitlab.local.tenstorrent.com:riscv/dv/vector-crack-wrapper.git",
    )
