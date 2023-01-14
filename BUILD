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
        "@boost//:lexical_cast",
        "@zug//:zug",
        "@cereal//:cereal",
    ],
    includes = [".", "lager/"],
    visibility = ["//visibility:public"],
)
