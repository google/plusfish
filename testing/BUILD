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
    name = "request_mock",
    testonly = 1,
    hdrs = ["request_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//:request",
        "//:response",
        "//util:html_fingerprint",
    ],
)

cc_library(
    name = "selective_auditor_mock",
    testonly = 1,
    hdrs = ["selective_auditor_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//audit:selective_auditor",
    ],
)

cc_library(
    name = "curl_mock",
    testonly = 1,
    hdrs = ["curl_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//:curl",
    ],
)

cc_library(
    name = "crawler_mock",
    testonly = 1,
    hdrs = ["crawler_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//:crawler",
        "//:datastore",
        "//audit:selective_auditor",
    ],
)


cc_library(
    name = "datastore_mock",
    testonly = 1,
    hdrs = ["datastore_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//:datastore",
    ],
)

cc_library(
    name = "hidden_objects_finder_mock",
    testonly = 1,
    hdrs = ["hidden_objects_finder_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//:hidden_objects_finder",
    ],
)

cc_library(
    name = "http_client_mock",
    testonly = 1,
    hdrs = ["http_client_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//:http_client",
    ],
)

cc_library(
    name = "reporter_mock",
    testonly = 1,
    hdrs = ["reporter_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//:pivot",
        "//proto:security_check_cc_proto",
        "//report:reporter",
    ],
)

cc_library(
    name = "security_check_mock",
    testonly = 1,
    hdrs = ["security_check_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//audit:security_check",
        "//proto:issue_details_cc_proto",
    ],
)

cc_library(
    name = "response_matcher_mock",
    testonly = 1,
    hdrs = ["response_matcher_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//audit:response_matcher",
    ],
)

cc_library(
    name = "generator_mock",
    testonly = 1,
    hdrs = ["generator_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//audit:generator",
    ],
)

cc_library(
    name = "ratelimiter_mock",
    testonly = 1,
    hdrs = ["ratelimiter_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//util:ratelimiter",
    ],
)

cc_library(
    name = "matcher_mock",
    testonly = 1,
    hdrs = ["matcher_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//audit/matchers:matcher",
    ],
)

cc_library(
    name = "fake_file_writer",
    testonly = 1,
    hdrs = ["fake_file_writer.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//util:file_writer",
    ],
)

cc_library(
    name = "matcher_factory_mock",
    testonly = 1,
    hdrs = ["matcher_factory_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//audit/matchers:matcher",
        "//audit/matchers:matcher_factory",
    ],
)


cc_library(
    name = "gumbo_filter_mock",
    testonly = 1,
    hdrs = ["gumbo_filter_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "@com_google_gumbo//:gumbo",
        "//parsers:gumbo_filter",
    ],
)

cc_library(
    name = "clock_mock",
    testonly = 1,
    hdrs = ["clock_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//util:clock",
    ],
)

cc_library(
    name = "not_found_detector_mock",
    testonly = 1,
    hdrs = ["not_found_detector_mock.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "//:not_found_detector",
    ],
)
