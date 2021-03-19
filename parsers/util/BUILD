package(default_visibility = ["//visibility:public"])

cc_library(
    name = "html_name",
    hdrs = ["html_name.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_library(
    name = "scrape_util",
    srcs = ["scrape_util.cc"],
    hdrs = ["scrape_util.h"],
    deps = [
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/strings",
        "//proto:http_common_cc_proto",
        "//proto:issue_details_cc_proto",
        "@com_google_re2//:re2",
        "//opensource/deps/base:base",
        "@com_github_google_glog//:glog",
    ],
)

cc_test(
    name = "scrape_util_test",
    size = "small",
    srcs = ["scrape_util_test.cc"],
    deps = [
        ":scrape_util",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/container:node_hash_set",
        "//proto:http_common_cc_proto",
        "//proto:issue_details_cc_proto",
    ],
)

cc_library(
    name = "escaping",
    srcs = ["escaping.cc"],
    hdrs = ["escaping.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_test(
    name = "escaping_test",
    size = "small",
    srcs = ["escaping_test.cc"],
    deps = [
        ":escaping",
        "@com_google_googletest//:gtest_main",
    ],
)


