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

#ifndef THIRD_PARTY_PLUSFISH_UTIL_HTML_FINGERPRINT_H_
#define THIRD_PARTY_PLUSFISH_UTIL_HTML_FINGERPRINT_H_

#include <string>

#include "base/integral_types.h"
#include "base/macros.h"

// The maximum amount of buckets to use for word length counting.
static const uint8 kMaxFingerprintSize = 16;
// The maximum percentage in difference allowed per bucket when comparing two
// fingerprints.
static const uint8 kMaxFingerprintBucketDiff = 5;
// The maximum percentage of different size words across all buckets.
static const uint8 kMaxFingerprintTotalDiff = 3;
// The maximum allowed of times a bucket comparison fail is accepted.
static const uint8 kMaxFingerprintBucketFails = 3;

namespace plusfish {

// A fuzzy HTML fingerprinter that bases equalness on word size distribution.
// The more words from a document are fed; the more accurate the comparison will
// be.
//
// The goal of this fingerprinter is to determine if documents look similar
// while allowing differences to exist. It is intentially simplistic for low
// memory consumption and high performance.
//
// Based on the skipfish fingerprint implementation.

class HtmlFingerprint {
 public:
  HtmlFingerprint();
  ~HtmlFingerprint() {}

  // Returns the array containing the counters per word size.
  const uint32_t* GetWordSizeCnt() const { return word_size_cnt_; }

  // Compare against the given fingerprint and returns true if they are similar
  // enough.
  bool Equals(const HtmlFingerprint& fp);
  // Add a word to the fingerprint.
  void AddWord(const std::string& word);
  // Output the bucket values into the string "output".
  void ToString(std::string* output) const;

 private:
  uint32_t word_size_cnt_[kMaxFingerprintSize];

  DISALLOW_COPY_AND_ASSIGN(HtmlFingerprint);
};

}  // namespace plusfish

#endif  // THIRD_PARTY_PLUSFISH_UTIL_HTML_FINGERPRINT_H_
