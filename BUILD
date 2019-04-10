package(default_visibility = ["//visibility:public"])

genrule(
    name = "lager-config",
    srcs = ["lager/config.hpp.in"],
    outs = ["lager/config.hpp"],
    cmd = "cp $< $@",
)

cc_library(
    name = "lager",
    include_prefix = "lager",
    strip_include_prefix = "lager",
    hdrs = glob([
        "lager/**/*.hpp",
    ]) + [":lager-config"],
    deps = ["@boost//:hana"],
)
