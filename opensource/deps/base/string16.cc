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

#include "base/string16.h"

#include <string.h>

#ifdef WIN32

#error This file should not be used on 2-byte wchar_t systems
// If this winds up being needed on 2-byte wchar_t systems, either the
// definitions below can be used, or the host system's wide character
// functions like wmemcmp can be wrapped.

#else  // !WIN32

namespace base {

int c16memcmp(const char16* s1, const char16* s2, size_t n) {
  // We cannot call memcmp because that changes the semantics.
  while (n-- > 0) {
    if (*s1 != *s2) {
      // We cannot use (*s1 - *s2) because char16 is unsigned.
      return ((*s1 < *s2) ? -1 : 1);
    }
    ++s1;
    ++s2;
  }
  return 0;
}

size_t c16len(const char16* s) {
  const char16 *s_orig = s;
  while (*s) {
    ++s;
  }
  return s - s_orig;
}

const char16* c16memchr(const char16* s, char16 c, size_t n) {
  while (n-- > 0) {
    if (*s == c) {
      return s;
    }
    ++s;
  }
  return 0;
}

char16* c16memmove(char16* s1, const char16* s2, size_t n) {
  return reinterpret_cast<char16*>(memmove(s1, s2, n * sizeof(char16)));
}

char16* c16memcpy(char16* s1, const char16* s2, size_t n) {
  return reinterpret_cast<char16*>(memcpy(s1, s2, n * sizeof(char16)));
}

char16* c16memset(char16* s, char16 c, size_t n) {
  char16 *s_orig = s;
  while (n-- > 0) {
    *s = c;
    ++s;
  }
  return s_orig;
}

}  // namespace base

template class std::basic_string<char16, base::string16_char_traits>;

#endif  // WIN32
