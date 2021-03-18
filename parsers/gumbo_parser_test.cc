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

#include "parsers/gumbo_parser.h"

#include <memory>
#include <string>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "gtest/gtest.h"
#include <gumbo.h>
#include "third_party/gumbo/test_utils.h"
#include "testing/gumbo_filter_mock.h"

using testing::_;
using testing::Eq;
using testing::Field;

namespace plusfish {

// This is a basic test to ensure the wrapper works. Gumbo itself already has
// plenty of tests: no need to replicate these here.
class GumboTest : public ::testing::Test {
 public:
  GumboTest() { gumbo_.reset(new GumboParser()); }

 protected:
  std::unique_ptr<GumboParser> gumbo_;
};

TEST_F(GumboTest, ParsesHtmlOkTest) {
  const GumboOutput* output = gumbo_->Parse("<html></html>");
  ASSERT_FALSE(output == nullptr);
  ASSERT_FALSE(gumbo_->output() == nullptr);

  GumboNode* html_node = GetChild(output->document, 0);
  ASSERT_EQ(GUMBO_NODE_ELEMENT, html_node->type);
  EXPECT_EQ(GUMBO_TAG_HTML, GetTag(html_node));
  gumbo_->DestroyCurrentOutput();
}

TEST_F(GumboTest, DestroyCurrentOutputIsFatalTest) {
  ASSERT_FALSE(gumbo_->Parse("<html></html>") == nullptr);
  gumbo_->DestroyCurrentOutput();
  ASSERT_TRUE(gumbo_->output() == nullptr);
}

TEST_F(GumboTest, ParseAttributesOk) {
  const GumboOutput* output = gumbo_->Parse("<html foo='bar'></html>");
  ASSERT_FALSE(output == nullptr);

  ASSERT_EQ(GUMBO_NODE_ELEMENT, output->root->type);
  EXPECT_EQ(1, output->root->v.element.attributes.length);

  GumboAttribute* attr =
      gumbo_->GetAttribute(&output->root->v.element.attributes, "foo");
  ASSERT_FALSE(attr == nullptr);
  ASSERT_STREQ(attr->value, "bar");
  gumbo_->DestroyCurrentOutput();
}

TEST_F(GumboTest, ParseAttributesNOk) {
  const GumboOutput* output = gumbo_->Parse("<html></html>");
  ASSERT_FALSE(output == nullptr);

  ASSERT_EQ(GUMBO_NODE_ELEMENT, output->root->type);
  GumboAttribute* attr =
      gumbo_->GetAttribute(&output->root->v.element.attributes, "foo");
  ASSERT_TRUE(attr == nullptr);
  gumbo_->DestroyCurrentOutput();
}

TEST_F(GumboTest, ParsesCallDestroyCurrentOutputsTwice) {
  gumbo_->Parse("<html>");
  ASSERT_FALSE(gumbo_->output() == nullptr);
  gumbo_->DestroyCurrentOutput();
  ASSERT_TRUE(gumbo_->output() == nullptr);
  gumbo_->DestroyCurrentOutput();
}

TEST_F(GumboTest, FilterDocumentOk) {
  std::vector<GumboFilter*> filters;
  std::unique_ptr<testing::MockGumboFilter> test_filter(
      new testing::MockGumboFilter());
  filters.emplace_back(test_filter.get());

  EXPECT_CALL(*test_filter,
              ParseElement(Field(&GumboElement::tag, Eq(GUMBO_TAG_HTML))));
  EXPECT_CALL(*test_filter,
              ParseElement(Field(&GumboElement::tag, Eq(GUMBO_TAG_HEAD))));
  EXPECT_CALL(*test_filter,
              ParseElement(Field(&GumboElement::tag, Eq(GUMBO_TAG_BODY))));
  EXPECT_CALL(*test_filter, ParseComment(_));
  EXPECT_CALL(*test_filter, ParseText(_));

  gumbo_->Parse("<html><head><body>text<!-- comment -->");
  gumbo_->FilterDocument(filters);
  gumbo_->DestroyCurrentOutput();
}

TEST_F(GumboTest, FilterDocumentWithMultipleFiltersOk) {
  std::vector<GumboFilter*> filters;
  std::unique_ptr<testing::MockGumboFilter> test_filter1(
      new testing::MockGumboFilter());
  std::unique_ptr<testing::MockGumboFilter> test_filter2(
      new testing::MockGumboFilter());
  filters.emplace_back(test_filter1.get());
  filters.emplace_back(test_filter2.get());

  EXPECT_CALL(*test_filter1, ParseElement(_)).Times(3);
  EXPECT_CALL(*test_filter2, ParseElement(_)).Times(3);
  gumbo_->Parse("<html><head><body>");
  gumbo_->FilterDocument(filters);
  gumbo_->DestroyCurrentOutput();
}

TEST_F(GumboTest, FilterDocumentWithoutContent) {
  std::vector<GumboFilter*> filters;
  std::unique_ptr<testing::MockGumboFilter> test_filter(
      new testing::MockGumboFilter());
  filters.emplace_back(test_filter.get());
  gumbo_->Parse("");
  gumbo_->FilterDocument(filters);
  gumbo_->DestroyCurrentOutput();
}

TEST_F(GumboTest, FillAttributeMapOk) {
  std::map<std::string, std::string> output_map;
  const GumboOutput* output = gumbo_->Parse("<html foo='bar' oh='yeh'></html>");
  ASSERT_FALSE(output == nullptr);
  ASSERT_EQ(GUMBO_NODE_ELEMENT, output->root->type);

  gumbo_->FillAttributeMap(output->root->v.element, &output_map);

  EXPECT_EQ(2, output->root->v.element.attributes.length);
  EXPECT_EQ(2, output_map.size());
  EXPECT_STREQ(output_map.find("foo")->second.c_str(), "bar");
  EXPECT_STREQ(output_map.find("oh")->second.c_str(), "yeh");
  gumbo_->DestroyCurrentOutput();
}

TEST_F(GumboTest, FillAttributeMapWithoutValue) {
  std::map<std::string, std::string> output_map;
  const GumboOutput* output = gumbo_->Parse("<html empty></html>");
  ASSERT_FALSE(output == nullptr);
  ASSERT_EQ(GUMBO_NODE_ELEMENT, output->root->type);

  gumbo_->FillAttributeMap(output->root->v.element, &output_map);

  EXPECT_EQ(1, output->root->v.element.attributes.length);
  EXPECT_EQ(1, output_map.size());
  EXPECT_STREQ(output_map.find("empty")->second.c_str(), "");
  gumbo_->DestroyCurrentOutput();
}

}  // namespace plusfish
