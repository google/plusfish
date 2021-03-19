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

#ifndef PLUSFISH_UTIL_FILEWRITER_H_
#define PLUSFISH_UTIL_FILEWRITER_H_

#include <fstream>
#include <string>

#include "opensource/deps/base/macros.h"

namespace plusfish {

// This is a utility class for writing to files.
class FileWriter {
 public:
  FileWriter() {}
  virtual ~FileWriter();

  // Open the given file for writing.
  virtual bool Open(const std::string& filename);
  // Append the string to the open file.
  virtual void WriteString(const std::string& string_to_write);
  // Close the file.
  virtual void Close();

 private:
  std::ofstream outfile_;

  DISALLOW_COPY_AND_ASSIGN(FileWriter);
};

}  // namespace plusfish

#endif  // PLUSFISH_UTIL_FILEWRITER_
