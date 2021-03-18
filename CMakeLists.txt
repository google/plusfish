# Copyright 2020 Google LLC. All Rights Reserved.
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

cmake_minimum_required(VERSION 3.11)
project(Plusfish)
set (CMAKE_CXX_STANDARD 11)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
include("abseil.cmake")
include("googleurl.cmake")

# Check libcurl dependencies
set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb -DNDEBUG=1")

add_subdirectory(proto)
add_subdirectory(audit)
add_subdirectory(report)
add_subdirectory(parsers)

add_dependencies(audit proto)
add_dependencies(report proto)
add_dependencies(parsers proto)

include_directories(
  ${CURL_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  opensource/deps/
  opensource/deps/third_party/
  opensource/deps/third_party/googleurl
  ${googleurl_SOURCE_DIR}
  ${abseil_SOURCE_DIR}
  ${abseil_SOURCE_DIR}/absl
  )

# Build curl client code.
set(CURL_SOURCES curl_http_client.cc curl_http_client.h curl.cc curl.h util/curl_util.h)
add_library(curl_client ${CURL_SOURCES})
target_link_libraries(curl_client ${CURL_LIBRARIES} proto)


set(PF_SOURCES
  crawler.cc
  crawler.h
  datastore.cc
  datastore.h
  hidden_objects_finder.cc
  hidden_objects_finder.h
  http_client.h
  not_found_detector.cc
  not_found_detector.h
  pivot.cc
  pivot.h
  plusfish.cc
  plusfish_cli.cc
  plusfish.h
  request.cc
  request.h
  request_handler.h
  response.cc
  response.h
  util/config.cc
  util/config.h
  util/http_util.cc
  util/http_util.h
  util/file_writer.cc
  util/file_writer.h
  util/ratelimiter.h
  util/simpleratelimiter.cc
  util/simpleratelimiter.h
  util/url.cc
  util/url.h
  util/html_fingerprint.cc
  util/html_fingerprint.h
  )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(plusfish ${PF_SOURCES})

target_link_libraries(plusfish curl_client absl::base absl::flags
  absl::flags_parse absl::strings absl::memory absl::random_random
  absl::synchronization absl::node_hash_set absl::node_hash_map absl::container
  absl::hash absl::synchronization audit parsers report gurl Threads::Threads
  proto "-lglog -lre2 -lprotobuf -lgumbo" )

