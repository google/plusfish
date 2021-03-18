// Copyright 2020 Google LLC. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PLUSFISH_UTIL_CONFIG_H_
#define PLUSFISH_UTIL_CONFIG_H_

#include <string>

#include "proto/security_check.pb.h"

namespace plusfish {

class SecurityCheckConfig;

namespace util {

// Load security check configs from the given path into the given config
// message. Globbing is allowed so "config/*.asciipb" will result in all
// matching files from that directory to be loaded.
// Returns false on failure and true on success.
// Does not take ownership.
bool LoadCheckConfigs(const std::string& path, SecurityCheckConfig* config);

// Load requests from asciipb into the collection.
bool LoadRequestsConfig(const std::string& file,
                        HttpRequestCollection* collection);
}  // namespace util
}  // namespace plusfish

#endif  // PLUSFISH_UTIL_CONFIG_H_
