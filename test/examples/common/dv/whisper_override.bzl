"""Derive a per-core cosim whisper config from //test/examples/common/dv:whisper.json."""

def whisper_json_override(name, out, substitutions, visibility = None):
    """Exact-string substitutions on the shared whisper.json; the greps fail
    the build if an anchor string is missing (i.e. the shared value changed).
    """
    checks = " && ".join(["grep -q '\"{}\"' $<".format(old) for old in substitutions])
    seds = " ".join(["-e 's/\"{}\"/\"{}\"/'".format(old, new) for old, new in substitutions.items()])
    native.genrule(
        name = name,
        srcs = ["//test/examples/common/dv:whisper.json"],
        outs = [out],
        cmd = "{} && sed {} $< > $@".format(checks, seds),
        visibility = visibility,
    )
