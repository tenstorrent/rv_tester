WHISPER_LOCAL_DEFINES = ["LZ4_COMPRESS"]

def whisper_client_register_gen(name, topology, visibility = None):
    native.cc_library(
        name = name,
        srcs = ["@rv_tester//src/cosim/whisper_if:whisper_client_register.cpp"],
        deps = [
            "@rv_tester//src/cosim/whisper_if:whisper_if",
            topology,
        ],
        local_defines = WHISPER_LOCAL_DEFINES,
        alwayslink = True,
        visibility = visibility,
    )
