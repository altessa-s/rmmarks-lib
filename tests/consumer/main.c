/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */

#include <rmmarks/rmmarks.h>
#include <string.h>

int main(void) {
  rmm_page_box band = {0, 0, 1, 1};
  rmm_placed_text word = {RMM_STR("portable"), band};
  rmm_line line = {&word, 1};
  rmm_passages* passages = nullptr;
  rmm_status status =
      rmm_text_passages(&band, 1, &line, 1, nullptr, 0, &passages, nullptr);
  if (status != RMM_OK) return 1;
  int result = passages->count == 1 && passages->items[0].text.length == 8 &&
                       memcmp(passages->items[0].text.bytes, "portable", 8) == 0
                   ? 0
                   : 2;
  rmm_passages_free(passages);
  return result;
}
