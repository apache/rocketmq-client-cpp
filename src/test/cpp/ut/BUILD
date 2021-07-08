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
    name = "event_test",
    srcs = [
        "EventTest.cpp",
    ],
    deps = [
        "//external:libuv",
        "@com_google_googletest//:gtest_main",
    ],
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
    name = "curl_test",
    srcs = [
        "CurlTest.cpp",
    ],
    deps = [
        "@curl//:curl",
        "@com_google_googletest//:gtest_main",
    ],
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

cc_test(
    name = "thread_pool_test",
    srcs = [
        "ThreadPoolTest.cpp",
    ],
    deps = [
        "@com_github_progschj_ThreadPool//:ThreadPool",
        "@com_github_gabime_spdlog//:spdlog",
        "@com_google_googletest//:gtest_main",
    ]
)