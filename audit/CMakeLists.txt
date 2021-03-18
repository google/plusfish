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

cmake_minimum_required(VERSION 3.10)

set (CMAKE_CXX_STANDARD 11)

include("../abseil.cmake")

include_directories(../ ../opensource/deps/ ../opensource/deps/third_party/
  ${abseil_SOURCE_DIR} ${googleurl_SOURCE_DIR})

add_subdirectory(matchers)
add_subdirectory(util)

# Build curl client code.
set(SOURCES
      generator.h
      generic_generator.cc
      generic_generator.h
      generic_response_matcher.cc
      generic_response_matcher.h
      generic_security_check.cc
      generic_security_check.h
      passive_auditor.cc
      passive_auditor.h
      response_matcher.h
      response_time_check.cc
      response_time_check.h
      security_check.h
      security_check_runner.cc
      security_check_runner.h
      selective_auditor.cc
      selective_auditor.h
  )
add_library(audit ${SOURCES})
target_link_libraries(audit audit_util matchers)







