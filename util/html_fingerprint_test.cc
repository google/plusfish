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

#include "util/html_fingerprint.h"

#include "gtest/gtest.h"

namespace plusfish {

TEST(HtmlFingerprintTest, MatchingExactOk) {
  HtmlFingerprint fp;
  fp.AddWord("this");
  fp.AddWord("is");
  fp.AddWord("a");
  fp.AddWord("test");

  HtmlFingerprint fp2;
  fp2.AddWord("this");
  fp2.AddWord("is");
  fp2.AddWord("a");
  fp2.AddWord("test");

  EXPECT_TRUE(fp.Equals(fp2));
}

TEST(HtmlFingerprintTest, MatchingAlmostTheSameOk) {
  // Not enough bucket mismatches to cause a fail. Also not enough total
  // different to cause a mismatch.
  HtmlFingerprint fp;
  HtmlFingerprint fp2;

  for (int i = 0; i < 98; i++) {
    fp.AddWord("this");
    fp2.AddWord("this");
  }

  fp2.AddWord("works");
  fp2.AddWord("alright");

  EXPECT_TRUE(fp.Equals(fp2));
}

TEST(HtmlFingerprintTest, MatchingFailsOnGlobalDiff) {
  // Not enough bucket mismatches to cause a fail but enough to mismatch on the
  // total difference.
  HtmlFingerprint fp;
  HtmlFingerprint fp2;

  for (int i = 0; i < 96; i++) {
    fp.AddWord("this");
    fp2.AddWord("this");
  }

  fp2.AddWord("works");
  fp2.AddWord("alright");
  fp2.AddWord("or");
  fp2.AddWord("not");

  EXPECT_FALSE(fp.Equals(fp2));
}

TEST(HtmlFingerprintTest, MatchingFailsOnBuckets) {
  // 4 words are of different length. This causes 4 bucket fails and therefore
  // the comparison results in a failure.
  HtmlFingerprint fp;
  fp.AddWord("this");

  HtmlFingerprint fp2;
  fp2.AddWord("is");
  fp2.AddWord("not");
  fp2.AddWord("a");
  fp2.AddWord("test");
  fp2.AddWord("maybe");

  EXPECT_FALSE(fp.Equals(fp2));
}

}  // namespace plusfish
