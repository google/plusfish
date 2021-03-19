load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

proto_library(
    name = "http_request",
    srcs = ["http_request.proto"],
)

cc_proto_library(
    name = "http_request_cc_proto",
    deps = [":http_request"],
)

proto_library(
    name = "http_response",
    srcs = ["http_response.proto"],
    deps = [":http_common"],
)

cc_proto_library(
    name = "http_response_cc_proto",
    deps = [":http_response"],
)

proto_library(
    name = "http_common",
    srcs = ["http_common.proto"],
)
cc_proto_library(
    name = "http_common_cc_proto",
    deps = [":http_common"],
)

proto_library(
    name = "generator",
    srcs = ["generator.proto"],
)

cc_proto_library(
    name = "generator_cc_proto",
    deps = [":generator"],
)

proto_library(
    name = "issue_details",
    srcs = ["issue_details.proto"],
    deps = [
        ":http_request",
        ":http_response",
        ":severity",
    ],
)

cc_proto_library(
    name = "issue_details_cc_proto",
    deps = [":issue_details"],
)

proto_library(
    name = "matching",
    srcs = ["matching.proto"],
)

cc_proto_library(
    name = "matching_cc_proto",
    deps = [":matching"],
)

proto_library(
    name = "security_check",
    srcs = ["security_check.proto"],
    deps = [
        ":advisory",
        ":generator",
        ":issue_details",
        ":matching",
    ],
)

cc_proto_library(
    name = "security_check_cc_proto",
    deps = [":security_check"],
)

proto_library(
    name = "report",
    srcs = ["report.proto"],
    deps = [
        ":http_request",
        ":http_response",
        ":issue_details",
    ],
)

cc_proto_library(
    name = "report_cc_proto",
    deps = [":report"],
)

proto_library(
    name = "advisory",
    srcs = ["advisory.proto"],
    deps = [
        ":issue_details",
        ":severity",
    ],
)

cc_proto_library(
    name = "advisory_cc_proto",
    deps = [":advisory"],
)

proto_library(
    name = "severity",
    srcs = ["severity.proto"],
)

cc_proto_library(
    name = "severity_cc_proto",
    deps = [":severity"],
)
