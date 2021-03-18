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

#include <cstdint>
#include <cstdlib>
#include <string>

#include "base/integral_types.h"
#include <glog/logging.h>

namespace plusfish {

HtmlFingerprint::HtmlFingerprint() {
  for (int i = 0; i < kMaxFingerprintSize; i++) {
    word_size_cnt_[i] = 0;
  }
}

void HtmlFingerprint::AddWord(const std::string& word) {
  word_size_cnt_[word.length() % kMaxFingerprintSize]++;
}

void HtmlFingerprint::ToString(std::string* output) const {
  for (int i = 0; i < kMaxFingerprintSize; i++) {
    output->append(std::to_string(word_size_cnt_[i]) + " ");
  }
}

bool HtmlFingerprint::Equals(const HtmlFingerprint& fp) {
  const uint32_t* compare_wc = fp.GetWordSizeCnt();

  uint8 failed_buckets = 0;
  uint32 total_diff = 0;
  uint32 total_scale = 0;

  for (int i = 0; i < kMaxFingerprintSize; i++) {
    int32_t diff = word_size_cnt_[i] - compare_wc[i];
    uint32_t scale = word_size_cnt_[i] + compare_wc[i];

    if (abs(diff) >= 1 + ((scale * kMaxFingerprintBucketDiff) / 100)) {
      if (++failed_buckets > kMaxFingerprintBucketFails) {
        DLOG(INFO) << "Too many failed buckets.";
        return false;
      }
    }

    total_diff += abs(diff);
    total_scale += scale;
  }

  if (total_diff >= 1 + ((total_scale * kMaxFingerprintTotalDiff) / 100)) {
    DLOG(INFO) << "Global diff too large.";
    return false;
  }
  return true;
}

}  // namespace plusfish
