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

#ifndef PLUSFISH_TESTING_REPORTER_MOCK_H_
#define PLUSFISH_TESTING_REPORTER_MOCK_H_

#include "gmock/gmock.h"
#include "pivot.h"
#include "proto/security_check.pb.h"
#include "report/reporter.h"

namespace plusfish {
namespace testing {

class MockReporter : public ReporterInterface {
 public:
  MOCK_METHOD0(Init, bool());
  MOCK_METHOD2(ReportPivot, void(const Pivot* pivot, int depth));
  MOCK_CONST_METHOD1(ReportSecurityConfig,
                     void(const SecurityCheckConfig& config));
};

}  // namespace testing
}  // namespace plusfish

#endif  // PLUSFISH_TESTING_REPORTER_MOCK_H_
