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
    name = "reporter",
    hdrs = ["reporter.h"],
    deps = [
        "//opensource/deps/base:base",
        "//util:file_writer",
    ],
)

cc_library(
    name = "text_reporter",
    srcs = ["text_reporter.cc"],
    hdrs = ["text_reporter.h"],
    deps = [
        ":reporter",
        "//opensource/deps/base:base",
        "//:pivot",
        "@com_github_google_glog//:glog",
        "//proto:http_request_cc_proto",
        "//proto:http_response_cc_proto",
        "//proto:issue_details_cc_proto",
        "//util:file_writer",
    ],
)

cc_test(
    name = "text_reporter_test",
    size = "small",
    srcs = ["text_reporter_test.cc"],
    deps = [
        ":text_reporter",
        "//opensource/deps/base:base",
        "@com_google_googletest//:gtest_main",
        "//:pivot",
        "//:request",
        "//proto:issue_details_cc_proto",
        "//testing:fake_file_writer",
    ],
)

cc_library(
    name = "json_reporter",
    srcs = ["json_reporter.cc"],
    hdrs = ["json_reporter.h"],
    deps = [
        ":reporter",
        "//opensource/deps/base:base",
        "@com_github_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "//:pivot",
        "//proto:http_request_cc_proto",
        "//proto:http_response_cc_proto",
        "//proto:report_cc_proto",
        "//proto:security_check_cc_proto",
        "//util:file_writer",
    ],
)

cc_library(
    name = "reporter_factory",
    srcs = ["reporter_factory.cc"],
    hdrs = ["reporter_factory.h"],
    deps = [
        ":json_reporter",
        ":reporter",
        ":text_reporter",
        "@com_github_google_glog//:glog",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/flags:flag",
        "//proto:report_cc_proto",
        "//util:file_writer",
    ],
)

cc_test(
    name = "reporter_factory_test",
    size = "small",
    srcs = ["reporter_factory_test.cc"],
    deps = [
        ":reporter",
        ":reporter_factory",
        "//opensource/deps/base:base",
        "@com_google_googletest//:gtest_main",
        "//proto:report_cc_proto",
    ],
)

