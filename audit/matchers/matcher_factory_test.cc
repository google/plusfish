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

#include "audit/matchers/matcher_factory.h"
#include "gtest/gtest.h"
#include "audit/matchers/matcher.h"
#include "proto/matching.pb.h"

namespace plusfish {

TEST(MatcherFactoryTest, FactoryReturnsMatcher) {
  MatcherFactory factory;
  MatchRule_Match match;
  match.set_method(MatchRule_Method_CONTAINS);
  std::unique_ptr<MatcherInterface> matcher(factory.GetMatcher(match));
  ASSERT_TRUE(matcher);
}

TEST(MatcherFactoryTest, FactoryReturnsNullptr) {
  MatcherFactory factory;
  MatchRule_Match match;
  match.set_method(MatchRule_Method_NONE);
  ASSERT_EQ(nullptr, factory.GetMatcher(match));
}

}  // namespace plusfish
