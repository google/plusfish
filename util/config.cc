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

#include "util/config.h"

#include <glob.h>    // glob(), globfree()
#include <string.h>  // memset()

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include "proto/http_request.pb.h"
#include "proto/security_check.pb.h"

namespace plusfish {
namespace util {

// Credit to:
// https://stackoverflow.com/questions/8401777/simple-glob-in-c-on-unix-system
bool glob_path(const std::string& pattern,
               std::vector<std::string>* filenames) {
  // glob struct resides on the stack
  glob_t glob_result;
  memset(&glob_result, 0, sizeof(glob_result));

  // do the glob operation
  int return_value = glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
  if (return_value != 0) {
    globfree(&glob_result);
    return false;
  }

  for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
    filenames->push_back(std::string(glob_result.gl_pathv[i]));
  }

  // cleanup
  globfree(&glob_result);
  return true;
}

bool ReadProtoFileContent(const std::string& filename, std::string* output) {
  std::ifstream in(filename);
  if (!in) {
    LOG(ERROR) << "The protobuf file appears invalid: " << filename;
    return false;
  }

  std::string content((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
  *output = content;
  return true;
}

bool LoadCheckConfigs(const std::string& path,
                      SecurityCheckConfig* checks_config) {
  std::vector<std::string> files;
  if (!glob_path(path, &files)) {
    LOG(ERROR) << "The configuration file std::string seems invalid: " << path;
    return false;
  }

  if (files.empty()) {
    LOG(ERROR) << "The path does not match any existing files: " << path;
    return false;
  }

  for (const std::string& config_file : files) {
    LOG(INFO) << "Loading security checks from: " << config_file;
    plusfish::SecurityCheckConfig tmp_proto;

    std::string contents;
    if (!ReadProtoFileContent(config_file, &contents)) {
      LOG(WARNING) << "Unable to parse proto from file " << config_file;
    }

    google::protobuf::TextFormat::ParseFromString(contents, &tmp_proto);

    checks_config->MergeFrom(tmp_proto);
  }
  return true;
}

bool LoadRequestsConfig(const std::string& file,
                        HttpRequestCollection* collection) {
  std::ifstream in(file);
  if (!in) {
    LOG(ERROR) << "The requests file appears invalid: " << file;
    return false;
  }

  std::string contents;
  if (!ReadProtoFileContent(file, &contents)) {
    LOG(WARNING) << "Unable to parse proto from file " << file;
  }

  return google::protobuf::TextFormat::ParseFromString(contents, collection);
}

}  // namespace util
}  // namespace plusfish
