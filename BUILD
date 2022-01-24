package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lager",
    hdrs = glob([
        "lager/**/*.hpp",
    ]),
    deps = [
        "@boost//:hana",
        "@boost//:intrusive",
        "@boost//:intrusive_ptr",
        "@zug//:zug",
        "@cereal//:cereal",
    ],
    includes = [".", "lager/"],
    visibility = ["//visibility:public"],
)
