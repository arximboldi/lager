package(default_visibility = ["//visibility:public"])

genrule(
    name = "lager-config",
    srcs = ["lager/config.hpp.in"],
    outs = ["lager/config.hpp"],
    cmd = "cp $< $@",
)

cc_library(
    name = "lager",
    hdrs = glob([
        "lager/**/*.hpp",
    ]) + [":lager-config"],
    deps = [
        "@boost//:hana",
        "@boost//:signals2",
        "@zug//:zug",
    ],
    includes = [".", "lager/"],
    visibility = ["//visibility:public"],
)
