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

#include "util/file_writer.h"

#include <fstream>
#include <string>

#include <glog/logging.h>

namespace plusfish {

FileWriter::~FileWriter() {
  if (outfile_.is_open()) {
    Close();
  }
}

bool FileWriter::Open(const std::string& filename) {
  outfile_.open(filename, std::ofstream::out);
  return outfile_.is_open();
}

void FileWriter::WriteString(const std::string& string_to_write) {
  DCHECK(outfile_.is_open());
  outfile_.write(string_to_write.c_str(), string_to_write.length());
}

void FileWriter::Close() { outfile_.close(); }

}  // namespace plusfish
