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
    name = "audit_util",
    srcs = ["audit_util.cc"],
    hdrs = ["audit_util.h"],
    deps = [ 
        "//opensource/deps/base:base",
        "//parsers/util:escaping",
        "//proto:generator_cc_proto",
    ],  
)

cc_test(
    name = "audit_util_test",
    size = "small",
    srcs = ["audit_util_test.cc"],
    deps = [ 
        ":audit_util",
        "@com_google_googletest//:gtest_main",
        "//proto:generator_cc_proto",
    ],  
)

cc_library(
    name = "issue_util",
    srcs = ["issue_util.cc"],
    hdrs = ["issue_util.h"],
    deps = [
        "//opensource/deps/base:base",
        "@com_google_absl//absl/container:node_hash_set",
        "//proto:issue_details_cc_proto",
    ],
)

cc_test(
    name = "issue_util_test",
    srcs = ["issue_util_test.cc"],
    deps = [
        ":issue_util",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/container:node_hash_set",
        "//proto:issue_details_cc_proto",
    ],
)

