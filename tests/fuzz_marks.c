/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */

#include <string.h>

#include "rmmarks/rmmarks.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 4) return 0;
  rmm_page_box bands[16];
  rmm_placed_text words[16];
  rmm_line lines[16];
  size_t n = data[0] % 16 + 1, at = 1;
  for (size_t i = 0; i < n; i += 1) {
    double c[4];
    for (size_t j = 0; j < 4; j += 1) {
      uint8_t byte = at < size ? data[at++] : 0;
      c[j] = (double)byte / 128 - .5;
    }
    bands[i] = (rmm_page_box){c[0], c[1], c[2], c[3]};
    size_t length = at < size ? data[at++] % 32 : 0;
    if (length > size - at) length = size - at;
    words[i] = (rmm_placed_text){{(const char*)data + at, length}, bands[i]};
    at += length;
    lines[i] = (rmm_line){words + i, 1};
  }
  rmm_band_reading alone = {0, lines, n};
  rmm_passages* p = nullptr;
  rmm_status status =
      rmm_text_passages(bands, n, lines, n, &alone, 1, &p, nullptr);
  if (status == RMM_OK) {
    rmm_blocks* blocks = nullptr;
    status = rmm_text_blocks(p->items, p->count, &blocks, nullptr);
    if (status == RMM_OK) rmm_blocks_free(blocks);
    rmm_passages_free(p);
  }
  rmm_mark_strip strip = {bands[0], bands[n - 1]};
  size_t number = 0;
  rmm_readings* r = nullptr;
  status = rmm_text_brought(lines, n, &strip, 1, &number, 1, &r, nullptr);
  if (status == RMM_OK) rmm_readings_free(r);
  return 0;
}
