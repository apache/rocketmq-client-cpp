"""Load dependencies needed to compile and test the RocketMQ library as a 3rd-party consumer."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")

def rocketmq_deps():
    """Loads dependencies need to compile and test the RocketMQ library."""
    native.bind(
        name = "opentelementry_api",
        actual = "@com_github_opentelemetry//api:api",
    )

    if "com_google_googletest" not in native.existing_rules():
         http_archive(
             name = "com_google_googletest",
             sha256 = "b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5",
             strip_prefix = "googletest-release-1.11.0",
             urls = [
                 "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz",
             ],
         )

    if "com_github_gulrak_filesystem" not in native.existing_rules():
        http_archive(
            name = "com_github_gulrak_filesystem",
            strip_prefix = "filesystem-1.5.0",
            sha256 = "eb6f3b0739908ad839cde68885d70e7324db191b9fad63d9915beaa40444d9cb",
            urls = [
                "https://github.com/gulrak/filesystem/archive/v1.5.0.tar.gz",
            ],
            build_file = "@org_apache_rocketmq//third_party:filesystem.BUILD",
        )

    if "com_github_progschj_ThreadPool" not in native.existing_rules():
        new_git_repository(
            name = "com_github_progschj_ThreadPool",
            remote = "https://github.com/progschj/ThreadPool.git",
            branch = "master",
            build_file = "@org_apache_rocketmq//third_party:ThreadPool.BUILD"
        )

    if "com_github_gabime_spdlog" not in native.existing_rules():
        http_archive(
            name = "com_github_gabime_spdlog",
            strip_prefix = "spdlog-1.8.2",
            sha256 = "e20e6bd8f57e866eaf25a5417f0a38a116e537f1a77ac7b5409ca2b180cec0d5",
            urls = [
                "https://github.com/gabime/spdlog/archive/v1.8.2.tar.gz",
            ],
            build_file = "@org_apache_rocketmq//third_party:spdlog.BUILD",
        )

    if "com_github_fmtlib_fmt" not in native.existing_rules():
        http_archive(
            name = "com_github_fmtlib_fmt",
            sha256 = "5cae7072042b3043e12d53d50ef404bbb76949dad1de368d7f993a15c8c05ecc",
            strip_prefix = "fmt-7.1.3",
            urls = [
                "https://github.com/fmtlib/fmt/archive/7.1.3.tar.gz",
            ],
            build_file = "@org_apache_rocketmq//third_party:fmtlib.BUILD",
        )

    if "com_google_protobuf" not in native.existing_rules():
        http_archive(
            name = "com_google_protobuf",
            sha256 = "36f81e03a0702f8f935fffd5a486dac1c0fc6d4bae1cd02c7a32448ad6e63bcb",
            strip_prefix = "protobuf-3.17.2",
            urls = [
                "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.17.2.tar.gz",
            ],
        )

    if "rules_proto_grpc" not in native.existing_rules():
        http_archive(
            name = "rules_proto_grpc",
            sha256 = "7954abbb6898830cd10ac9714fbcacf092299fda00ed2baf781172f545120419",
            strip_prefix = "rules_proto_grpc-3.1.1",
            urls = ["https://github.com/rules-proto-grpc/rules_proto_grpc/archive/3.1.1.tar.gz"],
        )

    if "curl" not in native.existing_rules():
        http_archive(
            name = "curl",
            strip_prefix = "curl-7.73.0",
            sha256 = "ba98332752257b47b9dea6d8c0ad25ec1745c20424f1dd3ff2c99ab59e97cf91",
            urls = ["https://curl.haxx.se/download/curl-7.73.0.tar.gz"],
            build_file = "@org_apache_rocketmq//third_party:curl.BUILD",
        )

    if "libuv" not in native.existing_rules():
        http_archive(
            name = "libuv",
            sha256 = "dfb4fe1ff0b47340978490a14bf253475159ecfcbad46ab2a350c78f9ce3360f",
            strip_prefix = "libuv-15ae750151ac9341e5945eb38f8982d59fb99201",
            urls = [
                "https://storage.googleapis.com/grpc-bazel-mirror/github.com/libuv/libuv/archive/15ae750151ac9341e5945eb38f8982d59fb99201.tar.gz",
                "https://github.com/libuv/libuv/archive/15ae750151ac9341e5945eb38f8982d59fb99201.tar.gz",
            ],
            build_file = "@org_apache_rocketmq//third_party:libuv.BUILD",
        )

    if "com_google_absl" not in native.existing_rules():
        http_archive(
            name = "com_google_absl",
            sha256 = "59b862f50e710277f8ede96f083a5bb8d7c9595376146838b9580be90374ee1f",
            strip_prefix = "abseil-cpp-20210324.2",
            urls = [
                "https://github.com/abseil/abseil-cpp/archive/refs/tags/20210324.2.tar.gz",
            ],
        )

    if "com_github_grpc_grpc" not in native.existing_rules():
        http_archive(
            name = "com_github_grpc_grpc",
            strip_prefix = "grpc-1.38.0",
            sha256 = "abd9e52c69000f2c051761cfa1f12d52d8b7647b6c66828a91d462e796f2aede",
            urls = ["https://github.com/grpc/grpc/archive/v1.38.0.tar.gz"],
        )

    if "com_github_opentelemetry" not in native.existing_rules():
        http_archive(
            name = "com_github_opentelemetry",
            sha256 = "2b5b423096c6e486ec23e77d0d75b28e32f86e7cdf6e7560da686c71c69908d3",
            strip_prefix = "opentelemetry-cpp-1.0.0-rc2",
            urls = [
                "https://github.com/open-telemetry/opentelemetry-cpp/archive/refs/tags/v1.0.0-rc2.tar.gz",
            ],
        )