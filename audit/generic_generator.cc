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

#include "audit/generic_generator.h"

#include <vector>

#include <glog/logging.h>
#include "google/protobuf/repeated_field.h"
#include "audit/util/audit_util.h"
#include "proto/generator.pb.h"
#include "request.h"

namespace {
// Whether or not to inject in a specific HttpRequest field. Returns true if the
// field name is explicitly matched against a whitelist. Also returns true if no
// whitelist exists (allowing all). Else returns false.
bool ShouldInjectInField(const plusfish::GeneratorRule_PayloadTarget& target,
                         const std::string& name) {
  // When no fields are specified, we inject in all.
  if (target.target_name_size() == 0) {
    return true;
  }
  for (const std::string& target_name : target.target_name()) {
    if (strcasecmp(target_name.c_str(), name.c_str()) == 0) {
      return true;
    }
  }
  return false;
}
}  // namespace

namespace plusfish {

GenericGenerator::GenericGenerator(const GeneratorRule& rule)
    : generator_rule_(rule) {
  DLOG(INFO) << "Create request generator with rule: " << rule.DebugString();
}

GenericGenerator::~GenericGenerator() {}

google::protobuf::RepeatedPtrField<HttpRequest_RequestField>*
GenericGenerator::GetMutableFieldsForType(int target_type,
                                          HttpRequest* req_proto) {
  if (target_type == GeneratorRule_PayloadTarget_TargetType_UNKNOWN) {
    LOG(ERROR) << "Unknown payload target type whilst getting field values.";
  }

  if (target_type == GeneratorRule_PayloadTarget_TargetType_URL_PARAMS &&
      req_proto->param_size()) {
    return req_proto->mutable_param();
  }

  if (target_type == GeneratorRule_PayloadTarget_TargetType_BODY_PARAMS &&
      req_proto->body_param_size()) {
    return req_proto->mutable_body_param();
  }

  if (target_type == GeneratorRule_PayloadTarget_TargetType_HEADERS &&
      req_proto->header_size()) {
    return req_proto->mutable_header();
  }

  if (target_type == GeneratorRule_PayloadTarget_TargetType_PATH_ELEMENTS &&
      req_proto->path_size()) {
    return req_proto->mutable_path();
  }

  return nullptr;
}

int GenericGenerator::Generate(
    const Request* base_request,
    std::vector<std::unique_ptr<Request>>* requests) {
  int request_count = 0;
  DLOG(INFO) << "Generating requests for: " << base_request->url();
  google::protobuf::RepeatedPtrField<HttpRequest_RequestField>* fields = nullptr;

  for (const auto& payload_target : generator_rule_.target()) {
    // Create a copy of the base request proto.
    HttpRequest req_proto(base_request->proto());

    fields = GetMutableFieldsForType(payload_target.type(), &req_proto);
    if (fields == nullptr) {
      DLOG(INFO) << "No fields found for payload target type: "
                 << payload_target.type();
      continue;
    }

    request_count += AppendMutatedFieldRequests(
        base_request->id(), fields, req_proto, payload_target, requests);
  }
  return request_count;
}

int GenericGenerator::AppendMutatedFieldRequests(
    const int64 parent_id,
    google::protobuf::RepeatedPtrField<HttpRequest_RequestField>* fields,
    const HttpRequest& req_proto,
    const GeneratorRule_PayloadTarget& payload_target,
    std::vector<std::unique_ptr<Request>>* requests) {
  int request_count = 0;

  if (payload_target.last_field_only() && !fields->empty()) {
    return AppendSingleMutatedFieldRequest(parent_id,
                                           fields->Mutable(fields->size() - 1),
                                           req_proto, payload_target, requests);
  }

  // Set the payload in the target fields.
  for (HttpRequest_RequestField& field : *fields) {
    // Check if we're supposed to inject in this fields or skip it.
    if (!field.has_name() ||
        !ShouldInjectInField(payload_target, field.name())) {
      continue;
    }

    request_count += AppendSingleMutatedFieldRequest(
        parent_id, &field, req_proto, payload_target, requests);
  }
  return request_count;
}

int GenericGenerator::AppendSingleMutatedFieldRequest(
    const int64 parent_id, HttpRequest_RequestField* field,
    const HttpRequest& req_proto,
    const GeneratorRule_PayloadTarget& payload_target,
    std::vector<std::unique_ptr<Request>>* requests) {
  int request_count = 0;

  // Copy the original value so we can restore the field to it's original
  // state later on. This simplifies the field modifications.
  const std::string original_value = field->value();
  const size_t method_count = generator_rule_.method_size();
  const size_t encoding_count = generator_rule_.encoding_size();

  for (int m_index = 0; m_index < method_count; ++m_index) {
    for (int e_index = 0; e_index < encoding_count; ++e_index) {
      // TODO: Move this to a payload generator.
      for (const std::string& payload : generator_rule_.payload().arg()) {
        std::string final_payload = util::GeneratePayloadString(
            original_value, payload, generator_rule_.method(m_index),
            generator_rule_.encoding(e_index));
        field->set_value(final_payload);
        field->set_modified(true);
        Request* new_request = new Request(req_proto);
        new_request->set_parent_id(parent_id);
        requests->push_back(std::unique_ptr<Request>(new_request));
        // Restore the original value.
        field->set_value(original_value);
        field->set_modified(false);
        ++request_count;
      }
    }
  }
  return request_count;
}

}  // namespace plusfish
