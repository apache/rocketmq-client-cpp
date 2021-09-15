"""Load dependencies needed to compile and test the RocketMQ library as a 3rd-party consumer."""
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

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

    if "com_github_gabime_spdlog" not in native.existing_rules():
        http_archive(
            name = "com_github_gabime_spdlog",
            strip_prefix = "spdlog-1.9.2",
            sha256 = "6fff9215f5cb81760be4cc16d033526d1080427d236e86d70bb02994f85e3d38",
            urls = [
                "https://github.com/gabime/spdlog/archive/refs/tags/v1.9.2.tar.gz",
            ],
            build_file = "@org_apache_rocketmq//third_party:spdlog.BUILD",
        )

    if "com_github_fmtlib_fmt" not in native.existing_rules():
        http_archive(
            name = "com_github_fmtlib_fmt",
            strip_prefix = "fmt-8.0.1",
            sha256 = "b06ca3130158c625848f3fb7418f235155a4d389b2abc3a6245fb01cb0eb1e01",
            urls = [
                "https://github.com/fmtlib/fmt/archive/refs/tags/8.0.1.tar.gz",
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
            strip_prefix = "grpc-1.39.0",
            sha256 = "b16992aa1c949c10d5d5ce2a62f9d99fa7de77da2943e643fb66dcaf075826d6",
            urls = ["https://github.com/grpc/grpc/archive/v1.39.0.tar.gz"],
        )

    if "io_opentelemetry_cpp" not in native.existing_rules():
        http_archive(
            name = "io_opentelemetry_cpp",
            sha256 = "24ba9b83f6cb8ba717ae30ebc570f5e8d0569008aee3c8b9a7ce6e4e1a5115b7",
            strip_prefix = "opentelemetry-cpp-1.0.0-rc4",
            urls = [
                "https://github.com/open-telemetry/opentelemetry-cpp/archive/refs/tags/v1.0.0-rc4.tar.gz",
            ],
        )

    maybe(
        http_archive,
        name = "com_github_opentelemetry_proto",
        build_file = "@io_opentelemetry_cpp//bazel:opentelemetry_proto.BUILD",
        sha256 = "08f090570e0a112bfae276ba37e9c45bf724b64d902a7a001db33123b840ebd6",
        strip_prefix = "opentelemetry-proto-0.6.0",
        urls = [
            "https://github.com/open-telemetry/opentelemetry-proto/archive/v0.6.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "asio",
        build_file = "@org_apache_rocketmq//third_party:asio.BUILD",
        strip_prefix = "asio-1.18.2",
        urls = [
            "https://github.com/lizhanhui/asio/archive/refs/tags/v1.18.2.tar.gz",
        ],
    )

    # Curl library
    maybe(
        http_archive,
        name = "com_github_curl",
        build_file = "@org_apache_rocketmq//third_party:curl.BUILD",
        strip_prefix = "curl-master",
        urls = ["https://github.com/curl/curl/archive/master.zip"],
    )