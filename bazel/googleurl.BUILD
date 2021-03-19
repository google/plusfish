
licenses(["notice"])  # Apache

exports_files(["COPYING"])

cc_library(
    name = "googleurl",
    srcs = [
      "src/gurl.cc",
      "src/url_canon.h",
      "src/url_canon_etc.cc",
      "src/url_canon_fileurl.cc",
      "src/url_canon_filesystemurl.cc",
      "src/url_canon_host.cc",
      "src/url_canon_internal.cc",
      "src/url_canon_internal.h",
      "src/url_canon_internal_file.h",
      "src/url_canon_ip.cc",
      "src/url_canon_ip.h",
      "src/url_canon_mailtourl.cc",
      "src/url_canon_path.cc",
      "src/url_canon_pathurl.cc",
      "src/url_canon_query.cc",
      "src/url_canon_relative.cc",
      "src/url_canon_stdstring.h",
      "src/url_canon_stdurl.cc",
      "src/url_common.h",
      "src/url_file.h",
      "src/url_util_internal.h",
      "src/url_parse.cc",
      "src/url_parse.h",
      "src/url_parse_file.cc",
      "src/url_parse_internal.h",
      "src/url_util.cc",
      "src/url_util.h",
      "base/basictypes.h",
      "base/logging.h",
      "base/scoped_ptr.h",
      "base/string16.cc",
      "base/string16.h",
      "fake/read_utf_char.cc",
      "fake/idn_to_ascii.cc",
    ],
    hdrs = [
      "src/gurl.h",
    ],
    includes = ["src"],
    visibility = ["//visibility:public"],
)
