# Security Checks overview

Most security checks in plusfish are implemented using configuration files.
These files are ascii protobuffers based and the structure is defined in
proto/security_checks.proto

In addition to reading this document; it also helps to understand the
configuration when you:

*  look at the security checks that are shipped with plusfish in the
./config/ directory since they make use of all functionality described
in this document.
*  look at the documentation in the protobuf files in the ./proto directory.

## Generator section

Security tests that require HTTP requests to be send have a GeneratorRule
section in which you can define the payloads, what fields to inject,
how to inject (e.g. before original value, behind it or replacing it),
how to encode the payload etc.

Below is an example of a generator rule with inline comment to document each
field and section.

```proto
  generator_rule <
    // Specify how to inject the payload in the target.
    // At least one entry is required.
    method: APPEND_TO_VALUE      // original_value<payload>
    method: PREFIX_VALUE         // <payload>original_value
    method: SET_VALUE            // <payload>

    // Specify how the payload should be encoded.
    // At least one entry required.
    encoding: NONE               // <payload>
    encoding: URL                // %3Cpayload%3E
    encoding: HTML               // &lt;payload&gt;

    // Where to inject the payloads. The target_name field can optionally be
    // given multiple times and will limit the injection to only fields matching
    // these names.
    // At least one target is required.

    target <
      type: HEADERS
      target_name: "User-Agent"  // Only inject in the user-agent header.
    >
    target <
      type: URL_PARAMS           // No target_name means inject in all possible
                                 // fields.
    >
    target <
      type: BODY_PARAMS          // HTTP post parameters.
    >
    target <
      type: PATH_ELEMENTS        // /path/element/
      last_field_only: true      // If the above is the path, only inject in
                                 // "element"
    >

    payload <
      type: STRING               // Right now this is the only supported type but
                                 // we have plans to add word and number
                                 // generators here.
      arg: "\\\"\"\\''"          // The payload to inject. You can specify
                                 // multiple here.
    >
  >
```
NOTE For each encoding, injection target and payload combination
one individual HTTP request is send to the server made. This can be a
significant amount when using multiple targets and encodings.

## Response matching section

Plusfish has many different "matchers" such as a RegexMatcher, ContainsMatcher,
PrefixMatcher, etc. The rules defined in this section of the security test
define "what matchers" and against "what part of the response".

It's important to know that matching is first done against the original request
and only if it doesn't return positive; done against the responses that are
returned from the HTTP requests that the generator rule launched.

The matcher protobuf is defined in proto/matching.proto


```proto
  matching_rule <
    // A "condition" performs matching against a section of the
    // response. You can specify multiple "condition" sections and all of
    // them need to fully match section for the security test be considered
    // successful (resulting in a vulnerability being reported).
    condition <
      // Indicates which part of the response to match against. Valid values
      // are RESPONSE_BODY, RESPONSE_HEADER_VALUE and REQUEST_URL.
      target: RESPONSE_BODY

      // Multiple match sections can be provided in which case they all need to
      // match on at least "one value" for the security issue to be considered
      // found.
      match <
        // The method specifies how the matching should be done. The following
        // values are values:
        //   CONTAINS: whether the given string is present in the content.
        //   REGEX:    whether the regex matches the content.
        //   EQUALS:   whether the content is the same.
        //   PREFIX:   whether the content starts with the given value.
        //   TIMING:   see timing section below
        method: CONTAINS
        // Multiple values can be specified and only one needs to match.
        value: "You have an error in your SQL syntax"
        value: "Incorrect syntax near the keyword"
      >

      // You can specify "field" at this level to limit the matching further.
      // Currenly this only works on HTTP header names (e.g. "Server").
    >
  >
```

### Response time matching
Plusfish measures the average response time of a page by requesting that page
several times. Response time matching in security checks is done by taking the
average response time of the page, adding it to the values given in the timing
range and checking if the actual response time (of the security test) is within
this range.

Below is how an example from the blind SQL injection tests that ship with
plusfish. The payload of those test try to delay the response time with an
additional 8 seconds.

```proto
  matching_rule <
    condition <
      target: RESPONSE_BODY       // Irrelevant
      match <
        method: TIMING
        timing <
          min_duration_ms: 8000
          max_duration_ms: 8500
        >
      >
    >
  >
```

If the average response time for the page is 200ms than this test will return
positive if the response time of the request (with the 8 second sleep payload)
takes between 8200 and 8700ms.





# Feeding HTTP requests upon startup

Besides starting plusfish with URLs on the command-line, you can also give it
HTTP requests that are written in ascii protobufs. The advantage of doing this
is that you can:

 * Specify endpoints that are hidden
 * Load HTTP POST requests with predefined data
 * Scope an entire scan to the proto HTTP requests when you disable crawling.

Use this feature to improve the coverage of the scan and/or do really targeted
fuzzing of endpoints.

## How to use it

```shell
./plusfish [...] --requests_asciipb_file requests.asciipb
...
...
```

An example file can be found as part of the unittest for this logic
(./util/testdata/test_requests.asciipb).  To see what fields you can define you
can also refer to the proto definition file in ./proto/http_request.proto.
