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

#include "report/reporter_factory.h"
#include <memory>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "report/reporter.h"

namespace plusfish {

class ReporterFactoryTest : public ::testing::Test {
 protected:
  ReporterFactoryTest() {}

  ReporterFactory factory_;
  std::unique_ptr<ReporterInterface> reporter_;
};

TEST_F(ReporterFactoryTest, ReturnsNullForUnknown) {
  std::string fake_name("doesntexist");
  EXPECT_EQ(nullptr, factory_.GetReporterByName(fake_name));
}

TEST_F(ReporterFactoryTest, ReturnsRealReporterByName) {
  std::string reporter_name("TEXT");
  reporter_.reset(factory_.GetReporterByName(reporter_name));
  EXPECT_FALSE(nullptr == reporter_);
}

}  // namespace plusfish
