
licenses(["notice"])  # Apache

exports_files(["COPYING"])

cc_library(
    name = "gumbo",
    srcs = [

        "src/error.c",
        "src/char_ref.c",
        "src/parser.c",
        "src/string_buffer.c",
        "src/utf8.c",
        "src/string_piece.c",
        "src/util.c",
        "src/vector.c",
        "src/attribute.c",
        "src/tag.c",
        "src/tokenizer.c",
    ],
    hdrs = [
        "src/token_type.h",
        "src/gumbo.h",
        "src/attribute.h",
        "src/vector.h",
        "src/tag_enum.h",
        "src/tag_gperf.h",
        "src/string_buffer.h",
        "src/util.h",
        "src/tokenizer.h",
        "src/char_ref.h",
        "src/tag_strings.h",
        "src/error.h",
        "src/tokenizer_states.h",
        "src/string_piece.h",
        "src/utf8.h",
        "src/parser.h",
        "src/insertion_mode.h",
        "src/tag_sizes.h",
    ],
    includes = ["src"],
    visibility = ["//visibility:public"],
)
