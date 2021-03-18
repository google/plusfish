#include "util/url.h"

#include <string>

#include <glog/logging.h>

namespace plusfish {
namespace util {

std::string StripUrlFileSuffix(const std::string& url) {
  std::string final_url = url;
  size_t params = final_url.find('?');

  // Remove everything following the ?
  if (params != std::string::npos) {
    final_url.resize(--params);
  }

  size_t scheme_end = final_url.find("//");
  size_t first_slash = final_url.find_first_of('/', scheme_end + 2);
  if (first_slash == std::string::npos) {
    return final_url + "/";
  }

  size_t trailing_slash = final_url.rfind('/');
  if (trailing_slash == final_url.length() - 1) {
    return final_url;
  }

  final_url.resize(trailing_slash + 1);
  return final_url;
}

}  // namespace util
}  // namespace plusfish
