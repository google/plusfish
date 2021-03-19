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

cc_library(
    name = "security_check",
    hdrs = ["security_check.h"],
    deps = [
        "//opensource/deps/base:base",
        "//:request_hdr",
        "//proto:issue_details_cc_proto",
    ],
)

cc_library(
    name = "generic_generator",
    srcs = ["generic_generator.cc"],
    hdrs = ["generic_generator.h"],
    deps = [
        ":generator",
        "//opensource/deps/base:base",
        "@com_google_protobuf//:protobuf",
        "//:request",
        "//audit/util:audit_util",
        "//proto:generator_cc_proto",
    ],
)

cc_library(
    name = "generator",
    hdrs = ["generator.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_library(
    name = "security_check_runner",
    srcs = ["security_check_runner.cc"],
    hdrs = ["security_check_runner.h"],
    deps = [
        ":security_check",
        "//opensource/deps/base:base",
        "//:http_client",
        "//:request",
        "//:request_handler",
        "//proto:issue_details_cc_proto",
    ],
)

cc_test(
    name = "security_check_runner_test",
    size = "small",
    srcs = ["security_check_runner_test.cc"],
    deps = [
        ":security_check_runner",
        "@com_google_googletest//:gtest_main",
        "//testing:http_client_mock",
        "//testing:request_mock",
        "//testing:security_check_mock",
        "//testing:selective_auditor_mock",
    ],
)

cc_library(
    name = "response_time_check",
    srcs = ["response_time_check.cc"],
    hdrs = ["response_time_check.h"],
    deps = [
        ":security_check",
        "//opensource/deps/base:base",
        "//:request",
        "//proto:http_request_cc_proto",
        "//proto:issue_details_cc_proto",
        "//proto:severity_cc_proto",
    ],
)


cc_library(
    name = "generic_security_check",
    srcs = ["generic_security_check.cc"],
    hdrs = ["generic_security_check.h"],
    deps = [
        ":generator",
        ":response_matcher",
        ":security_check",
        "//opensource/deps/base:base",
        "//:request",
        "//proto:issue_details_cc_proto",
        "//proto:security_check_cc_proto",
    ],
)

cc_test(
    name = "generic_security_check_test",
    size = "small",
    srcs = ["generic_security_check_test.cc"],
    deps = [
        ":generic_security_check",
        "@com_google_googletest//:gtest_main",
        "//:request",
        "//proto:security_check_cc_proto",
        "//testing:generator_mock",
        "//testing:response_matcher_mock",
    ],
)

cc_library(
    name = "response_matcher",
    hdrs = ["response_matcher.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_library(
    name = "generic_response_matcher",
    srcs = ["generic_response_matcher.cc"],
    hdrs = ["generic_response_matcher.h"],
    deps = [
        ":response_matcher",
        "//opensource/deps/base:base",
        "//:request",
        "//:response",
        "//audit/matchers:condition_matcher",
        "//audit/matchers:matcher",
        "//audit/matchers:matcher_factory",
        "//proto:matching_cc_proto",
    ],
)

cc_test(
    name = "generic_response_matcher_test",
    size = "small",
    srcs = ["generic_response_matcher_test.cc"],
    deps = [
        ":generic_response_matcher",
        "//opensource/deps/base:base",
        "@com_google_googletest//:gtest_main",
        "//:request",
        "//:response",
        "//proto:matching_cc_proto",
        "//testing:matcher_factory_mock",
        "//testing:matcher_mock",
        "//testing:request_mock",
    ],
)

cc_library(
    name = "selective_auditor",
    srcs = ["selective_auditor.cc"],
    hdrs = ["selective_auditor.h"],
    deps = [
        ":generic_generator",
        ":generic_response_matcher",
        ":generic_security_check",
        ":security_check",
        ":security_check_runner",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/synchronization",
        "//:http_client",
        "//audit/matchers:matcher_factory",
        "//proto:issue_details_cc_proto",
        "//proto:security_check_cc_proto",
    ],
)

cc_test(
    name = "selective_auditor_test",
    size = "small",
    srcs = ["selective_auditor_test.cc"],
    data = [
        "testdata/security_policy.asciipb",
        "testdata/security_policy_incorrect_matcher.asciipb",
        "testdata/security_policy_no_generator.asciipb",
        "testdata/security_policy_no_matcher.asciipb",
    ],
    deps = [
        ":selective_auditor",
        "@com_google_googletest//:gtest_main",
        "//testing:crawler_mock",
        "//testing:http_client_mock",
        "//testing:matcher_factory_mock",
        "//testing:matcher_mock",
        "//testing:request_mock",
        "//testing:security_check_mock",
    ],
)


cc_library(
    name = "passive_auditor",
    srcs = ["passive_auditor.cc"],
    hdrs = ["passive_auditor.h"],
    deps = [
        ":generic_response_matcher",
        "//opensource/deps/base:base",
        "//:request",
        "//audit/matchers:matcher_factory",
        "//proto:issue_details_cc_proto",
        "//proto:security_check_cc_proto",
    ],
)

cc_test(
    name = "passive_auditor_test",
    size = "small",
    srcs = ["passive_auditor_test.cc"],
    deps = [
        ":passive_auditor",
        "@com_google_googletest//:gtest_main",
        "//:response",
        "//proto:security_check_cc_proto",
        "//testing:matcher_factory_mock",
        "//testing:matcher_mock",
        "//testing:request_mock",
    ],
)

