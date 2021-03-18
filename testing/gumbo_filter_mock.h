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

#ifndef THIRD_PARTY_PLUSFISH_TESTING_GUMBO_FILTER_MOCK_H_
#define THIRD_PARTY_PLUSFISH_TESTING_GUMBO_FILTER_MOCK_H_

#include "gmock/gmock.h"
#include <gumbo.h>
#include "parsers/gumbo_filter.h"

namespace plusfish {
namespace testing {

class MockGumboFilter : public GumboFilter {
 public:
  MOCK_METHOD1(ParseElement, void(const GumboElement& node));
  MOCK_METHOD1(ParseComment, void(const GumboText& node));
  MOCK_METHOD1(ParseText, void(const GumboText& node));
};

}  // namespace testing
}  // namespace plusfish

#endif  // THIRD_PARTY_PLUSFISH_TESTING_GUMBO_FILTER_MOCK_H_
