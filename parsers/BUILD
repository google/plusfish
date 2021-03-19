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
    name = "html_name",
    hdrs = ["html_name.h"],
    deps = [
        "//opensource/deps/base:base",
    ],
)

cc_library(
    name = "gumbo_parser",
    srcs = ["gumbo_parser.cc"],
    hdrs = ["gumbo_parser.h"],
    deps = [
        ":gumbo_filter",
        "//opensource/deps/base:base",
        "@com_google_gumbo//:gumbo",
    ],
)

cc_library(
    name = "gumbo_filter",
    hdrs = ["gumbo_filter.h"],
    deps = [
        "//opensource/deps/base:base",
        "@com_google_gumbo//:gumbo",
    ],
)

cc_library(
    name = "gumbo_xss_filter",
    srcs = ["gumbo_xss_filter.cc"],
    hdrs = ["gumbo_xss_filter.h"],
    deps = [
        ":gumbo_filter",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_gumbo//:gumbo",
        "//:request",
        "//audit/util:issue_util",
        "//parsers/util:scrape_util",
        "//proto:issue_details_cc_proto",
    ],
)

cc_library(
    name = "gumbo_fingerprint_filter",
    srcs = ["gumbo_fingerprint_filter.cc"],
    hdrs = ["gumbo_fingerprint_filter.h"],
    deps = [
        ":gumbo_filter",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_gumbo//:gumbo",
        "//parsers/util:scrape_util",
        "//proto:issue_details_cc_proto",
        "//util:html_fingerprint",
    ],
)

cc_library(
    name = "gumbo_url_filter",
    srcs = ["gumbo_url_filter.cc"],
    hdrs = ["gumbo_url_filter.h"],
    deps = [
        ":gumbo_filter",
        ":gumbo_parser",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_gumbo//:gumbo",
        "//audit/util:issue_util",
        "//parsers/util:escaping",
        "//parsers/util:html_name",
        "//parsers/util:scrape_util",
        "//proto:issue_details_cc_proto",
    ],
)

cc_library(
    name = "gumbo_form_filter",
    srcs = ["gumbo_form_filter.cc"],
    hdrs = ["gumbo_form_filter.h"],
    deps = [
        ":gumbo_filter",
        ":gumbo_parser",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_gumbo//:gumbo",
        "//:request",
        "//audit/util:issue_util",
        "//parsers/util:escaping",
        "//parsers/util:html_name",
        "//parsers/util:scrape_util",
        "//proto:issue_details_cc_proto",
    ],
)

cc_library(
    name = "html_scraper",
    srcs = ["html_scraper.cc"],
    hdrs = ["html_scraper.h"],
    deps = [
        ":gumbo_filter",
        ":gumbo_fingerprint_filter",
        ":gumbo_form_filter",
        ":gumbo_parser",
        ":gumbo_url_filter",
        ":gumbo_xss_filter",
        ":html_parser",
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/memory",
        "@com_google_gumbo//:gumbo",
        "//:request",
        "//proto:issue_details_cc_proto",
        "//util:html_fingerprint",
    ],
)

cc_test(
    name = "html_scraper_test",
    size = "small",
    srcs = ["html_scraper_test.cc"],
    deps = [
        ":html_scraper",
        "//opensource/deps/base:base",
        "@com_google_googletest//:gtest_main",
        "//:request",
        "//:response",
        "//testing:request_mock",
    ],
)

cc_library(
    name = "html_parser",
    hdrs = ["html_parser.h"],
    deps = [
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "//:request",
    ],
)

