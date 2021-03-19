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
    name = "matcher",
    hdrs = ["matcher.h"],
    deps = [
        "//opensource/deps/base:base",
        "//:request",
        "//proto:matching_cc_proto",
    ],
)

cc_library(
    name = "matcher_factory",
    srcs = ["matcher_factory.cc"],
    hdrs = ["matcher_factory.h"],
    deps = [
        ":contains_matcher",
        ":equals_matcher",
        ":matcher",
        ":prefix_matcher",
        ":regex_matcher",
        ":timing_matcher",
        "//opensource/deps/base:base",
        "//proto:matching_cc_proto",
    ],
)

cc_test(
    name = "matcher_factory_test",
    size = "small",
    srcs = ["matcher_factory_test.cc"],
    deps = [
        ":matcher",
        ":matcher_factory",
        "@com_google_googletest//:gtest_main",
        "//proto:matching_cc_proto",
    ],
)

cc_library(
    name = "contains_matcher",
    srcs = ["contains_matcher.cc"],
    hdrs = ["contains_matcher.h"],
    deps = [
        ":matcher",
        "//opensource/deps/base:base",
        "//:request",
        "//proto:matching_cc_proto",
    ],
)

cc_test(
    name = "contains_matcher_test",
    size = "small",
    srcs = ["contains_matcher_test.cc"],
    deps = [
        ":contains_matcher",
        "@com_google_googletest//:gtest_main",
        "//proto:matching_cc_proto",
        "//testing:request_mock",
    ],
)

cc_library(
    name = "equals_matcher",
    srcs = ["equals_matcher.cc"],
    hdrs = ["equals_matcher.h"],
    deps = [
        ":matcher",
        "//opensource/deps/base:base",
        "//:request",
        "//proto:matching_cc_proto",
    ],
)

cc_test(
    name = "equals_matcher_test",
    size = "small",
    srcs = ["equals_matcher_test.cc"],
    deps = [
        ":equals_matcher",
        "@com_google_googletest//:gtest_main",
        "//proto:matching_cc_proto",
        "//testing:request_mock",
    ],
)

cc_library(
    name = "prefix_matcher",
    srcs = ["prefix_matcher.cc"],
    hdrs = ["prefix_matcher.h"],
    deps = [
        ":matcher",
        "//opensource/deps/base:base",
        "//:request",
        "//proto:matching_cc_proto",
    ],
)

cc_test(
    name = "prefix_matcher_test",
    size = "small",
    srcs = ["prefix_matcher_test.cc"],
    deps = [
        ":prefix_matcher",
        "@com_google_googletest//:gtest_main",
        "//proto:matching_cc_proto",
        "//testing:request_mock",
    ],
)

cc_library(
    name = "timing_matcher",
    srcs = ["timing_matcher.cc"],
    hdrs = ["timing_matcher.h"],
    deps = [
        ":matcher",
        "//opensource/deps/base:base",
        "//:request",
        "//proto:http_request_cc_proto",
        "//proto:matching_cc_proto",
    ],
)

cc_test(
    name = "timing_matcher_test",
    size = "small",
    srcs = ["timing_matcher_test.cc"],
    deps = [
        ":timing_matcher",
        "@com_google_googletest//:gtest_main",
        "//:request",
        "//proto:http_request_cc_proto",
        "//proto:matching_cc_proto",
        "//testing:datastore_mock",
    ],
)

cc_library(
    name = "regex_matcher",
    srcs = ["regex_matcher.cc"],
    hdrs = ["regex_matcher.h"],
    deps = [
        ":matcher",
        "//opensource/deps/base:base",
        "//:request",
        "//proto:matching_cc_proto",
        "@com_google_re2//:re2",
    ],
)

cc_test(
    name = "regex_matcher_test",
    size = "small",
    srcs = ["regex_matcher_test.cc"],
    deps = [
        ":regex_matcher",
        "@com_google_googletest//:gtest_main",
        "//proto:matching_cc_proto",
        "//testing:request_mock",
    ],
)

cc_library(
    name = "condition_matcher",
    srcs = ["condition_matcher.cc"],
    hdrs = ["condition_matcher.h"],
    deps = [
        ":matcher",
        "//opensource/deps/base:base",
        "//:request",
        "//proto:matching_cc_proto",
    ],
)

cc_test(
    name = "condition_matcher_test",
    size = "small",
    srcs = ["condition_matcher_test.cc"],
    deps = [
        ":condition_matcher",
        "@com_google_googletest//:gtest_main",
        "//proto:matching_cc_proto",
        "//testing:matcher_mock",
        "//testing:request_mock",
    ],
)

