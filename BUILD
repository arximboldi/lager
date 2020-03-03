package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lager",
    hdrs = glob([
        "lager/**/*.hpp",
    ]),
    deps = [
        "@boost//:hana",
        "@boost//:signals2",
        "@zug//:zug",
    ],
    includes = [".", "lager/"],
    visibility = ["//visibility:public"],
)
