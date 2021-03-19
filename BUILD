# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

exports_files([
    "LICENSE",
])


cc_library(
    name = "request_handler",
    hdrs = ["request_handler.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_library(
    name = "request_hdr",
    hdrs = ["request.h"],
    deps = [
        ":request_handler",
        ":response",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_googleurl//:googleurl",
        "//proto:http_request_cc_proto",
        "//proto:issue_details_cc_proto",
        "//util:html_fingerprint",
    ],
)

cc_library(
    name = "datastore_hdr",
    hdrs = ["datastore.h"],
    deps = [
        ":request_hdr",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/synchronization",
        "//proto:http_request_cc_proto",
        "//proto:issue_details_cc_proto",
        "//util:html_fingerprint",
        "@com_google_re2//:re2",
    ],
)

cc_library(
    name = "curl_http_client",
    srcs = ["curl_http_client.cc"],
    hdrs = ["curl_http_client.h"],
    deps = [
        ":curl",
        ":http_client",
        ":request",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/synchronization",
        "@com_github_curl_curl//:curl",
        "//proto:http_request_cc_proto",
        "//util:curl_util",
        "//util:ratelimiter",
        "//util:simpleratelimiter",
    ],
)

#cc_test(
#    name = "curl_http_client_test",
#    size = "small",
#    srcs = ["curl_http_client_test.cc"],
#    deps = [
#        ":curl_http_client",
#        ":request",
#        "//opensource/deps/base:base",
#        "@com_google_googletest//:gtest_main",
#        "@com_google_absl//absl/flags:flag",
#        "@com_github_curl_curl//:curl",
#        "//testing:curl_mock",
#        "//testing:ratelimiter_mock",
#        "//testing:request_mock",
#        "//util:curl_util",
#        "//util:http_util",
#    ],
#)


cc_library(
    name = "response",
    srcs = ["response.cc"],
    hdrs = ["response.h"],
    deps = [
        "//opensource/deps/base:base",
        "@com_google_absl//absl/strings",
        "@com_github_google_glog//:glog",
        "//proto:http_common_cc_proto",
        "//proto:http_response_cc_proto",
        "//util:html_fingerprint",
        "//util:http_util",
    ],
)

cc_library(
    name = "request",
    srcs = ["request.cc"],
    hdrs = ["request.h"],
    deps = [
        ":datastore_hdr",
        ":request_handler",
        ":response",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
        "@com_github_google_glog//:glog",
        "@com_google_googleurl//:googleurl",
        "//audit:security_check",
        "//proto:http_request_cc_proto",
        "//proto:issue_details_cc_proto",
        "//util:html_fingerprint",
        "//util:http_util",
    ],
)

cc_test(
    name = "request_test",
    srcs = ["request_test.cc"],
    deps = [
        ":request",
        ":request_handler",
        "//opensource/deps/base:base",
        "@com_google_googletest//:gtest_main",
        "//proto:http_request_cc_proto",
        "//testing:request_mock",
        "//testing:security_check_mock",
    ],
)

cc_library(
    name = "pivot",
    srcs = ["pivot.cc"],
    hdrs = ["pivot.h"],
    deps = [
        ":request",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/flags:flag",
        "//report:reporter",
    ],
)

cc_test(
    name = "pivot_test",
    size = "small",
    srcs = ["pivot_test.cc"],
    deps = [
        ":pivot",
        ":request",
        "@com_google_googletest//:gtest_main",
        "//testing:reporter_mock",
    ],
)

cc_library(
    name = "http_client",
    hdrs = ["http_client.h"],
    deps = [
        ":request",
        ":request_handler",
        "//opensource/deps/base:base",
    ],
)

cc_library(
    name = "curl",
    srcs = ["curl.cc"],
    hdrs = ["curl.h"],
    deps = [
        ":request",
        "//opensource/deps/base:base",
        "@com_github_curl_curl//:curl",
        "//util:curl_util",
        "//util:http_util",
    ],
)


cc_library(
    name = "crawler",
    srcs = ["crawler.cc"],
    hdrs = ["crawler.h"],
    deps = [
        ":datastore",
        ":http_client",
        ":request",
        ":request_handler",
        ":response",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/flags:flag",
        "//audit:passive_auditor",
        "//audit:selective_auditor",
        "//parsers:html_scraper",
        "//parsers/util:scrape_util",
        "//proto:http_common_cc_proto",
        "//proto:http_request_cc_proto",
        "//proto:http_response_cc_proto",
        "//util:http_util",
        "//util:config",
    ],
)


#cc_test(
#    name = "crawler_test",
#    size = "small",
#    srcs = ["crawler_test.cc"],
#    deps = [
#        ":crawler",
#        ":http_client",
#        ":request",
#        ":response",
#        "//opensource/deps/base:base",
#        "@com_google_googletest//:gtest_main",
#        "@com_google_absl//absl/flags:flag",
#        "//proto:http_response_cc_proto",
#        "//testing:datastore_mock",
#        "//testing:http_client_mock",
#        "//testing:reporter_mock",
#        "//testing:request_mock",
#        "//testing:selective_auditor_mock",
#    ],
#)

cc_library(
    name = "plusfish",
    srcs = ["plusfish.cc"],
    hdrs = ["plusfish.h"],
    deps = [
        ":crawler",
        ":datastore",
        ":hidden_objects_finder",
        ":http_client",
        ":not_found_detector",
        ":request",
        ":request_handler",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/synchronization",
        "//audit:selective_auditor",
        "//proto:security_check_cc_proto",
        "//report:reporter",
        "//report:reporter_factory",
        "//util:clock",
    ],
)

cc_test(
    name = "plusfish_test",
    size = "small",
    srcs = ["plusfish_test.cc"],
    deps = [
        ":hidden_objects_finder",
        ":plusfish",
        ":request_hdr",
        "//opensource/deps/base:base",
        "@com_google_googletest//:gtest_main",
        "//proto:security_check_cc_proto",
        "//testing:clock_mock",
        "//testing:crawler_mock",
        "//testing:datastore_mock",
        "//testing:hidden_objects_finder_mock",
        "//testing:http_client_mock",
        "//testing:not_found_detector_mock",
        "//testing:selective_auditor_mock",
        "//util:clock",
    ],
)

cc_test(
    name = "response_test",
    srcs = ["response_test.cc"],
    deps = [
        ":response",
        "@com_google_googletest//:gtest_main",
        "//proto:http_common_cc_proto",
        "//proto:http_response_cc_proto",
        "//util:html_fingerprint",
    ],
)

cc_library(
    name = "datastore",
    srcs = ["datastore.cc"],
    hdrs = ["datastore.h"],
    deps = [
        ":pivot",
        ":request",
        ":response",
        "//opensource/deps/base:base",
        "@com_google_protobuf//:protobuf",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/synchronization",
        "//proto:http_common_cc_proto",
        "//proto:http_request_cc_proto",
        "//proto:issue_details_cc_proto",
        "//proto:severity_cc_proto",
        "//util:html_fingerprint",
        "//util:http_util",
        "@com_google_re2//:re2",
    ],
)

#cc_test(
#    name = "datastore_test",
#    srcs = ["datastore_test.cc"],
#    deps = [
#        ":datastore",
#        ":request",
#        ":response",
#        "//opensource/deps/base:base",
#        "@com_google_googletest//:gtest_main",
#        "@com_google_absl//absl/flags:flag",
#        "//proto:http_response_cc_proto",
#        "//proto:issue_details_cc_proto",
#        "//report:reporter",
#        "//testing:crawler_mock",
#        "//testing:reporter_mock",
#        "//testing:request_mock",
#        "//util:html_fingerprint",
#    ],
#)

cc_library(
    name = "hidden_objects_finder",
    srcs = ["hidden_objects_finder.cc"],
    hdrs = ["hidden_objects_finder.h"],
    deps = [
        ":datastore_hdr",
        ":http_client",
        ":request",
        ":request_handler",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/strings",
        "//parsers:gumbo_filter",
        "//parsers:gumbo_fingerprint_filter",
        "//parsers:gumbo_parser",
        "//proto:http_response_cc_proto",
        "//proto:issue_details_cc_proto",
        "//util:html_fingerprint",
        "//util:url",
    ],
)

cc_test(
    name = "hidden_objects_finder_test",
    srcs = ["hidden_objects_finder_test.cc"],
    deps = [
        ":hidden_objects_finder",
        ":http_client",
        ":request",
        ":request_handler",
        ":response",
        "//opensource/deps/base:base",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/strings",
        "//parsers:gumbo_filter",
        "//parsers:gumbo_fingerprint_filter",
        "//parsers:gumbo_parser",
        "//proto:http_response_cc_proto",
        "//proto:issue_details_cc_proto",
        "//testing:request_mock",
        "//util:html_fingerprint",
        "//util:url",
    ],
)

cc_library(
    name = "not_found_detector",
    srcs = ["not_found_detector.cc"],
    hdrs = ["not_found_detector.h"],
    deps = [
        ":http_client",
        ":request",
        ":request_handler",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/random:distributions",
        "//parsers:gumbo_filter",
        "//parsers:gumbo_fingerprint_filter",
        "//parsers:gumbo_parser",
        "//util:html_fingerprint",
        "//util:url",
    ],
)

cc_test(
    name = "not_found_detector_test",
    size = "small",
    srcs = ["not_found_detector_test.cc"],
    deps = [
        ":not_found_detector",
        ":request",
        ":response",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_set",
        "//testing:http_client_mock",
        "//testing:request_mock",
        "//util:html_fingerprint",
    ],
)

cc_binary(
    name = "plusfish_cli",
    srcs = ["plusfish_cli.cc"],
    deps = [
        ":crawler",
        ":curl_http_client",
        ":datastore",
        ":hidden_objects_finder",
        ":http_client",
        ":not_found_detector",
        ":plusfish",
        ":request",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/flags:config",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/flags:usage",
        "@com_google_absl//absl/strings",
        "//audit:passive_auditor",
        "//audit:response_time_check",
        "//audit:selective_auditor",
        "//audit/matchers:matcher_factory",
        "//proto:http_request_cc_proto",
        "//proto:security_check_cc_proto",
        "//util:clock",
        "//util:config",
    ],
    linkopts = [
        "-lssl"
        ],
)


