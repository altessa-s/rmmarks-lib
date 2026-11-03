/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */

#include <float.h>

#include "marks_test.h"
#include "rmm_internal.h"

static void allocation_failures(void) {
  rmm_page_box bands[] = {B(0, 1, 0, 1), B(0, 1, 0, 1)};
  rmm_placed_text p = W("café", 0, 1, 0, 1),
                  q = W("cafe\u0301 autre", 0, 1, 0, 1);
  rmm_line line = {&p, 1}, cut = {&q, 1};
  rmm_band_reading alone = {0, &cut, 1};
  bool complete = false;
  for (size_t i = 0; i < 1000; i += 1) {
    rmm_passages sentinel = {0}, *out = &sentinel;
    rmm_error error;
    rmm_test_fail_after(i);
    rmm_status status =
        rmm_text_passages(bands, 2, &line, 1, &alone, 1, &out, &error);
    if (!rmm_test_did_fail()) {
      CHECK(status == RMM_OK);
      rmm_passages_free(out);
      complete = true;
      break;
    }
    CHECK(status == RMM_ERR_OUT_OF_MEMORY);
    CHECK(error.status == status);
    CHECK(out == &sentinel);
  }
  CHECK(complete);
  rmm_marked_passage passages[] = {{.place = bands[0]}, {.place = bands[1]}};
  complete = false;
  for (size_t i = 0; i < 1000; i += 1) {
    rmm_blocks sentinel = {0}, *out = &sentinel;
    rmm_test_fail_after(i);
    rmm_status status = rmm_text_blocks(passages, 2, &out, nullptr);
    if (!rmm_test_did_fail()) {
      CHECK(status == RMM_OK);
      rmm_blocks_free(out);
      complete = true;
      break;
    }
    CHECK(status == RMM_ERR_OUT_OF_MEMORY);
    CHECK(out == &sentinel);
  }
  CHECK(complete);
  rmm_mark_strip strip = {bands[0], bands[0]};
  size_t number = 4;
  complete = false;
  for (size_t i = 0; i < 1000; i += 1) {
    rmm_readings sentinel = {0}, *out = &sentinel;
    rmm_test_fail_after(i);
    rmm_status status =
        rmm_text_brought(&line, 1, &strip, 1, &number, 1, &out, nullptr);
    if (!rmm_test_did_fail()) {
      CHECK(status == RMM_OK);
      rmm_readings_free(out);
      complete = true;
      break;
    }
    CHECK(status == RMM_ERR_OUT_OF_MEMORY);
    CHECK(out == &sentinel);
  }
  CHECK(complete);
  rmm_recognized_line reading = {p.text, p.box};
  complete = false;
  for (size_t i = 0; i < 1000; i += 1) {
    rmm_passages sentinel = {0}, *out = &sentinel;
    rmm_test_fail_after(i);
    rmm_status status =
        rmm_text_passages_from_reading(bands, 2, &reading, 1, &out, nullptr);
    if (!rmm_test_did_fail()) {
      CHECK(status == RMM_OK);
      rmm_passages_free(out);
      complete = true;
      break;
    }
    CHECK(status == RMM_ERR_OUT_OF_MEMORY);
    CHECK(out == &sentinel);
  }
  CHECK(complete);
  rmm_test_fail_after(SIZE_MAX);
}
int main(void) {
  rmm_passages sentinel = {0}, *out = &sentinel;
  CHECK(rmm_text_passages(nullptr, 1, nullptr, 0, nullptr, 0, &out, nullptr) ==
        RMM_ERR_INVALID_ARGUMENT);
  CHECK(out == &sentinel);
  CHECK(rmm_text_passages(nullptr, 0, nullptr, 0, nullptr, 0, nullptr,
                          nullptr) == RMM_ERR_INVALID_ARGUMENT);
  rmm_page_box b = B(0, 1, 0, 1);
  CHECK(rmm_text_passages(&b, SIZE_MAX, nullptr, 0, nullptr, 0, &out,
                          nullptr) == RMM_ERR_INVALID_ARGUMENT);
  rmm_line invalid = {nullptr, 1};
  CHECK(rmm_text_passages(&b, 1, &invalid, 1, nullptr, 0, &out, nullptr) ==
        RMM_ERR_INVALID_ARGUMENT);
  rmm_placed_text p = {.text = {nullptr, 1}, .box = b};
  rmm_line line = {&p, 1};
  CHECK(rmm_text_passages(&b, 1, &line, 1, nullptr, 0, &out, nullptr) ==
        RMM_ERR_INVALID_ARGUMENT);
  p.text = (rmm_str){"\xff", 1};
  CHECK(rmm_text_passages(&b, 1, &line, 1, nullptr, 0, &out, nullptr) ==
        RMM_ERR_INVALID_UTF8);
  p.text = RMM_STR("word");
  p.box.y0 = NAN;
  CHECK(rmm_text_passages(&b, 1, &line, 1, nullptr, 0, &out, nullptr) ==
        RMM_ERR_INVALID_ARGUMENT);
  p.box = b;
  rmm_page_box bad = B(-DBL_MAX, DBL_MAX, 0, 1);
  CHECK(rmm_text_passages(&bad, 1, &line, 1, nullptr, 0, &out, nullptr) ==
        RMM_ERR_INVALID_ARGUMENT);
  rmm_band_reading duplicates[] = {{0, &line, 1}, {0, &line, 1}};
  CHECK(rmm_text_passages(&b, 1, &line, 1, duplicates, 2, &out, nullptr) ==
        RMM_ERR_INVALID_ARGUMENT);
  CHECK(out == &sentinel);
  rmm_error error = {0};
  out = nullptr;
  CHECK(rmm_text_passages(nullptr, 0, nullptr, 0, nullptr, 0, &out, &error) ==
        RMM_OK);
  CHECK(out && out->count == 0 && error.status == RMM_OK);
  rmm_passages_free(out);
  rmm_readings* readings = nullptr;
  CHECK(rmm_text_brought(nullptr, 0, nullptr, 0, nullptr, 0, &readings,
                         nullptr) == RMM_OK);
  CHECK(readings && readings->count == 0);
  rmm_readings_free(readings);
  rmm_arena* arena = rmm_arena_create();
  CHECK(arena != nullptr);
  CHECK(rmm_arena_alloc(arena, SIZE_MAX) == nullptr);
  CHECK(rmm_arena_calloc(arena, SIZE_MAX, 2) == nullptr);
  CHECK(rmm_arena_strndup(arena, "x", SIZE_MAX) == nullptr);
  rmm_arena_destroy(arena);
  allocation_failures();
  return rm_test_report("errors");
}
