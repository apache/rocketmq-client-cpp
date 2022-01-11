load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def deps():
    maybe(
        http_archive,
        name = "bazel_skylib",
        sha256 = "1dde365491125a3db70731e25658dfdd3bc5dbdfd11b840b3e987ecf043c7ca0",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/bazel_skylib/bazel_skylib-0.9.0.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/0.9.0/bazel_skylib-0.9.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_absl",
        sha256 = "dcf71b9cba8dc0ca9940c4b316a0c796be8fab42b070bb6b7cab62b48f0e66c4",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/abseil/abseil-cpp-20211102.0.tar.gz",
            "https://github.com/abseil/abseil-cpp/archive/refs/tags/20211102.0.tar.gz",
        ],
        strip_prefix = "abseil-cpp-20211102.0",
    )

    maybe(
        http_archive,
        name = "net_zlib_zlib",
        build_file = "@com_github_nelhage_rules_boost//:BUILD.zlib",
        sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
        strip_prefix = "zlib-1.2.11",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/zlib/zlib-1.2.11.tar.gz",
            "https://zlib.net/zlib-1.2.11.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "org_bzip_bzip2",
        build_file = "@com_github_nelhage_rules_boost//:BUILD.bzip2",
        sha256 = "ab5a03176ee106d3f0fa90e381da478ddae405918153cca248e682cd0c4a2269",
        strip_prefix = "bzip2-1.0.8",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/bzip2/bzip2-1.0.8.tar.gz",
            "https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "org_lzma_lzma",
        build_file = "@com_github_nelhage_rules_boost//:BUILD.lzma",
        sha256 = "71928b357d0a09a12a4b4c5fafca8c31c19b0e7d3b8ebb19622e96f26dbf28cb",
        strip_prefix = "xz-5.2.3",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/xz/xz-5.2.3.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_facebook_zstd",
        build_file = "@com_github_nelhage_rules_boost//:BUILD.zstd",
        sha256 = "dc05773342b28f11658604381afd22cb0a13e8ba17ff2bd7516df377060c18dd",
        strip_prefix = "zstd-1.5.1",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/zstd/zstd-1.5.1.tar.gz",
            "https://github.com/facebook/zstd/releases/download/v1.5.1/zstd-1.5.1.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "boost",
        build_file = "@com_github_nelhage_rules_boost//:BUILD.boost",
        patch_cmds = ["rm -f doc/pdf/BUILD"],
        patch_cmds_win = ["Remove-Item -Force doc/pdf/BUILD"],
        sha256 = "94ced8b72956591c4775ae2207a9763d3600b30d9d7446562c552f0a14a63be7",
        strip_prefix = "boost_1_78_0",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/boost/boost_1_78_0.tar.gz",
            "https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "openssl",
        sha256 = "6f640262999cd1fb33cf705922e453e835d2d20f3f06fe0d77f6426c19257308",
        strip_prefix = "boringssl-fc44652a42b396e1645d5e72aba053349992136a",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/boringssl/boringssl-fc44652a42b396e1645d5e72aba053349992136a.tar.gz",
            "https://github.com/google/boringssl/archive/fc44652a42b396e1645d5e72aba053349992136a.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_nelhage_rules_boost",
        sha256 = "d27971effef74594638ee6ee3688250b7ba76e5f8c59ad499c8506ac6007ac59",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/rules_boost/rules_boost-1.0.0-rocketmq.tar.gz",
            "https://github.com/lizhanhui/rules_boost/archive/refs/tags/1.0.0-rocketmq.tar.gz",
        ],
        strip_prefix = "rules_boost-1.0.0-rocketmq",
    )

    maybe(
        http_archive,
        name = "jsoncpp",
        sha256 = "28eb40ba95585c83a7d518dc71134930c3658d7f47c3f3cec34e3cf5fbc3f5f7",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/jsoncpp/jsoncpp-0.10.7.tar.gz",
        ],
        strip_prefix = "jsoncpp-0.10.7",
        build_file = "@com_github_apache_rocketmq_cpp_client//bazel:jsoncpp.BUILD",
    )

    maybe(
        http_archive,
        name = "libevent",
        sha256 = "7180a979aaa7000e1264da484f712d403fcf7679b1e9212c4e3d09f5c93efc24",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/libevent/libevent-release-2.1.12-stable.tar.gz",
            "https://github.com/libevent/libevent/archive/refs/tags/release-2.1.12-stable.tar.gz",
        ],
        strip_prefix = "libevent-release-2.1.12-stable",
        build_file = "@com_github_apache_rocketmq_cpp_client//bazel:libevent.BUILD",
    )

    maybe(
        http_archive,
        name = "rules_foreign_cc",
        sha256 = "bcd0c5f46a49b85b384906daae41d277b3dc0ff27c7c752cc51e43048a58ec83",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/rules_foreign_cc/rules_foreign_cc-0.7.1.tar.gz",
            "https://github.com/bazelbuild/rules_foreign_cc/archive/refs/tags/0.7.1.tar.gz",
        ],
        strip_prefix = "rules_foreign_cc-0.7.1",
    )

    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5",
        strip_prefix = "googletest-release-1.11.0",
        urls = [
            "https://shutian.oss-cn-hangzhou.aliyuncs.com/cdn/googletest/googletest-release-1.11.0.tar.gz",
            "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz",
        ],
    )