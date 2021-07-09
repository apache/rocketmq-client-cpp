load("@rules_cc//cc:defs.bzl", "cc_test", "cc_binary")
cc_test(
    name = "gtest_example",
    srcs = [
        "GTestExample.cpp",
    ],
    deps = [
        "@com_google_googletest//:gtest_main",
    ]
)

cc_test(
    name = "log_test",
    srcs = [
        "LogTest.cpp",
    ],
    deps = [
        "@com_github_gabime_spdlog//:spdlog",
        "@com_google_googletest//:gtest_main",
    ]
)

cc_test(
    name = "asan_demo",
    srcs = [
        "AsanDemo.cpp",
    ],
    deps = [
        "@com_google_googletest//:gtest_main",
    ]
)

cc_test(
    name = "absl_time_test",
    srcs = [
        "AbseilTimeTest.cpp",
    ],
    deps = [
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "hex_test",
    srcs = [
        "HexTest.cpp",
    ],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "zlib_test",
    srcs = [
        "ZlibTest.cpp",
    ],
    deps = [
        "@zlib//:zlib",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "mac_test",
    srcs = [
        "MacTest.cpp",
    ],
)