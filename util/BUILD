
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        "//opensource/deps/base:base",
        "//proto:http_request_cc_proto",
        "@com_google_protobuf//:protobuf",
        "//proto:security_check_cc_proto",
    ],
)

cc_library(
    name = "url",
    srcs = ["url.cc"],
    hdrs = ["url.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_test(
    name = "url_test",
    size = "small",
    srcs = ["url_test.cc"],
    deps = [
        ":url",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "curl_util",
    textual_hdrs = ["curl_util.h"],
    deps = [
        "//opensource/deps/base:base",
        "//:request",
    ],
)

cc_library(
    name = "html_fingerprint",
    srcs = ["html_fingerprint.cc"],
    hdrs = ["html_fingerprint.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_library(
    name = "http_util",
    srcs = ["http_util.cc"],
    hdrs = ["http_util.h"],
    deps = [
        "//opensource/deps/base:base",
        "@com_google_absl//absl/strings",
        "//proto:http_common_cc_proto",
    ],
)

cc_test(
    name = "http_util_test",
    size = "small",
    srcs = ["http_util_test.cc"],
    deps = [
        ":http_util",
        "@com_google_googletest//:gtest_main",
        "//proto:http_common_cc_proto",
    ],
)

cc_library(
    name = "ratelimiter",
    hdrs = ["ratelimiter.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_library(
    name = "simpleratelimiter",
    srcs = ["simpleratelimiter.cc"],
    hdrs = ["simpleratelimiter.h"],
    deps = [
        ":ratelimiter",
        "//opensource/deps/base:base",
        "@com_github_google_glog//:glog",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "simpleratelimiter_test",
    size = "small",
    srcs = ["simpleratelimiter_test.cc"],
    deps = [
        ":simpleratelimiter",
        "@com_google_googletest//:gtest_main",
    ],
)


cc_library(
    name = "file_writer",
    srcs = ["file_writer.cc"],
    hdrs = ["file_writer.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_library(
    name = "clock",
    hdrs = ["clock.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)


