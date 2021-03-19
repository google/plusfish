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

#ifndef PLUSFISH_UTIL_FAKEFILEWRITER_H_
#define PLUSFISH_UTIL_FAKEFILEWRITER_H_

#include <glog/logging.h>

#include "gmock/gmock.h"
#include "opensource/deps/base/macros.h"
#include "util/file_writer.h"

namespace plusfish {
namespace testing {

// This is a fake FileWriter implementation for testing.
class FakeFileWriter : public FileWriter {
 public:
  // Does not take ownership.
  explicit FakeFileWriter(std::string* file_content)
      : file_content_(file_content) {}
  ~FakeFileWriter() override {}

  MOCK_METHOD1(Open, bool(const std::string& filename));
  MOCK_METHOD0(Close, void());

  void WriteString(const std::string& string_to_write) override {
    DCHECK(file_content_);
    file_content_->append(string_to_write);
  }

 private:
  std::string* file_content_;

  DISALLOW_COPY_AND_ASSIGN(FakeFileWriter);
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_UTIL_FAKEFILEWRITER_
