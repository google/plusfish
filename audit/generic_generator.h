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

#ifndef PLUSFISH_AUDIT_GENERIC_GENERATOR_H_
#define PLUSFISH_AUDIT_GENERIC_GENERATOR_H_

#include "opensource/deps/base/macros.h"

#include <vector>

#include "google/protobuf/repeated_field.h"
#include "audit/generator.h"
#include "proto/generator.pb.h"
#include "request.h"

namespace plusfish {

class Request;

// A rule based HTTP request generator class.
//
// Example usage:
//   GeneratorRule rule;
//   GenericGenerator generator(&rule);
//   std::vector<std::unique_ptr<Request>> generated_requests;
//   generator.Generate(base_request, &generated_requests);
//   generator.Generate(other_request, &generated_requests);
//
// The generated_requests std::vector will now contain all requests that were
// derived from base_request and other_request.
class GenericGenerator : public GeneratorInterface {
 public:
  // The GeneratorRule defines how new requests should be derived from the
  // base_request's that are given to the Generate method.
  explicit GenericGenerator(const GeneratorRule& rule);
  ~GenericGenerator() override;

  // Uses base_request in combination with the GeneratorRule to create requests.
  // These requests are stored in the given requests std::vector and owned by
  // the caller. Returns the amount of requests that were added to the
  // std::vector.
  int Generate(const Request* base_request,
               std::vector<std::unique_ptr<Request>>* requests) override;

 private:
  const GeneratorRule generator_rule_;
  // Returns the request's fields to be processed for the Payload target_type.
  google::protobuf::RepeatedPtrField<HttpRequest_RequestField>*
  GetMutableFieldsForType(int target_type, HttpRequest* req_proto);

  // Given a set of mutable fields from a HttpRequest, adds mutated copies of
  // these fields to the provided requests std::vector.  They are mutated
  // according to the payloads defined for this generator.
  int AppendMutatedFieldRequests(
      const int64 parent_id,
      google::protobuf::RepeatedPtrField<HttpRequest_RequestField>* fields,
      const HttpRequest& req_proto,
      const GeneratorRule_PayloadTarget& payload_target,
      std::vector<std::unique_ptr<Request>>* requests);

  // Adds mutated copies of the given field to the provided requests
  // std::vector. They are mutated according to the payloads defined for this
  // generator. Returns 1 on success and 0 on failure. Does not take ownership.
  int AppendSingleMutatedFieldRequest(
      const int64 parent_id, HttpRequest_RequestField* field,
      const HttpRequest& req_proto,
      const GeneratorRule_PayloadTarget& payload_target,
      std::vector<std::unique_ptr<Request>>* requests);

  DISALLOW_COPY_AND_ASSIGN(GenericGenerator);
};

}  // namespace plusfish

#endif  // PLUSFISH_AUDIT_GENERIC_GENERATOR_H_
