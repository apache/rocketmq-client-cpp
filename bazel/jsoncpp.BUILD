licenses(["unencumbered"])  # Public Domain or MIT

exports_files(["LICENSE"])

# The recommended approach to integrating JsonCpp in your project is to include
# the amalgamated source. And we are using this way.
cc_library(
    name = "jsoncpp",
    srcs = [
        "jsoncpp.cpp",
    ],
    hdrs = glob(["json/*.h"]),
    copts = [
        "-DJSON_USE_EXCEPTION=0",
        "-DJSON_HAS_INT64",
    ],
    visibility = ["//visibility:public"],
)