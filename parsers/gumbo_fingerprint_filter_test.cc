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

#include "parsers/gumbo_fingerprint_filter.h"

#include <memory>

#include "file/base/fileutils.h"
#include "file/base/helpers.h"
#include "file/base/path.h"
#include "strings/case.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include "absl/strings/ascii.h"
#include <gumbo.h>
#include "parsers/gumbo_filter.h"
#include "parsers/gumbo_parser.h"
#include "response.h"
#include "testing/request_mock.h"
#include "util/html_fingerprint.h"
#include "util/gtl/map_util.h"

namespace plusfish {

class GumboFingerprintFilterTest : public ::testing::Test {
 protected:
  GumboFingerprintFilterTest() {}
  void SetUp() override {}

  const std::string GetContentFromFile(const std::string& filename) {
    std::string content;
    CHECK_OK(file::GetContents(
        file::JoinPath(absl::GetFlag(FLAGS_test_srcdir),
                       "google3/third_party/plusfish/parsers/testdata/",
                       filename),
        &content, file::Defaults()));

    return content;
  }

  void Parse(const std::string& content) { gumbo_->Parse(content); }

  std::string payload_;
  std::unique_ptr<GumboParser> gumbo_;
  std::unique_ptr<GumboFingerprintFilter> fp_filter_;
  std::vector<GumboFilter*> filters_;
};

TEST_F(GumboFingerprintFilterTest, GetFingerprintSameForIdenticalFiles) {
  std::string content1 = GetContentFromFile("one.html");
  std::string content2 = GetContentFromFile("one.html");

  auto fp1 = absl::make_unique<HtmlFingerprint>();
  auto fp2 = absl::make_unique<HtmlFingerprint>();

  fp_filter_ = absl::make_unique<GumboFingerprintFilter>(std::move(fp1));
  filters_.clear();
  filters_.emplace_back(fp_filter_.get());
  gumbo_ = absl::make_unique<GumboParser>();

  gumbo_->Parse(content1);
  gumbo_->FilterDocument(filters_);
  fp1 = fp_filter_->GetFingerprint();

  fp_filter_ = absl::make_unique<GumboFingerprintFilter>(std::move(fp2));
  filters_.clear();
  filters_.emplace_back(fp_filter_.get());
  gumbo_ = absl::make_unique<GumboParser>();

  gumbo_->Parse(content2);
  gumbo_->FilterDocument(filters_);
  fp2 = fp_filter_->GetFingerprint();

  EXPECT_TRUE(fp1->Equals(*fp2.get()));
}

TEST_F(GumboFingerprintFilterTest, GetFingerprintDifferent) {
  std::string content1 = GetContentFromFile("one.html");
  std::string content2 = GetContentFromFile("two.html");

  auto fp1 = absl::make_unique<HtmlFingerprint>();
  auto fp2 = absl::make_unique<HtmlFingerprint>();

  fp_filter_ = absl::make_unique<GumboFingerprintFilter>(std::move(fp1));
  filters_.clear();
  filters_.emplace_back(fp_filter_.get());
  gumbo_ = absl::make_unique<GumboParser>();

  gumbo_->Parse(content1);
  gumbo_->FilterDocument(filters_);
  fp1 = fp_filter_->GetFingerprint();

  fp_filter_ = absl::make_unique<GumboFingerprintFilter>(std::move(fp2));
  filters_.clear();
  filters_.emplace_back(fp_filter_.get());
  gumbo_ = absl::make_unique<GumboParser>();

  gumbo_->Parse(content2);
  gumbo_->FilterDocument(filters_);
  fp2 = fp_filter_->GetFingerprint();

  EXPECT_FALSE(fp1->Equals(*fp2.get()));
}

TEST_F(GumboFingerprintFilterTest, GetFingerprintSameForSimilarWordsFiles) {
  std::string content1 = GetContentFromFile("one.html");
  std::string content2 = GetContentFromFile("one-similar.html");

  auto fp1 = absl::make_unique<HtmlFingerprint>();
  auto fp2 = absl::make_unique<HtmlFingerprint>();

  fp_filter_ = absl::make_unique<GumboFingerprintFilter>(std::move(fp1));
  filters_.clear();
  filters_.emplace_back(fp_filter_.get());
  gumbo_ = absl::make_unique<GumboParser>();

  gumbo_->Parse(content1);
  gumbo_->FilterDocument(filters_);
  fp1 = fp_filter_->GetFingerprint();

  fp_filter_ = absl::make_unique<GumboFingerprintFilter>(std::move(fp2));
  filters_.clear();
  filters_.emplace_back(fp_filter_.get());
  gumbo_ = absl::make_unique<GumboParser>();

  gumbo_->Parse(content2);
  gumbo_->FilterDocument(filters_);
  fp2 = fp_filter_->GetFingerprint();

  EXPECT_TRUE(fp1->Equals(*fp2.get()));
}

TEST_F(GumboFingerprintFilterTest, GetFingerprintSameForSimilarHtmlFiles) {
  std::string content1 = GetContentFromFile("one.html");
  std::string content2 = GetContentFromFile("one-also-similar.html");

  auto fp1 = absl::make_unique<HtmlFingerprint>();
  auto fp2 = absl::make_unique<HtmlFingerprint>();

  fp_filter_ = absl::make_unique<GumboFingerprintFilter>(std::move(fp1));
  filters_.clear();
  filters_.emplace_back(fp_filter_.get());
  gumbo_ = absl::make_unique<GumboParser>();

  gumbo_->Parse(content1);
  gumbo_->FilterDocument(filters_);
  fp1 = fp_filter_->GetFingerprint();

  fp_filter_ = absl::make_unique<GumboFingerprintFilter>(std::move(fp2));
  filters_.clear();
  filters_.emplace_back(fp_filter_.get());
  gumbo_ = absl::make_unique<GumboParser>();

  gumbo_->Parse(content2);
  gumbo_->FilterDocument(filters_);
  fp2 = fp_filter_->GetFingerprint();

  EXPECT_TRUE(fp1->Equals(*fp2.get()));
}

}  // namespace plusfish
