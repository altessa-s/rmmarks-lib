/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */

#include "marks_test.h"

static rmm_readings* bring(const rmm_line* lines, size_t n,
                           const rmm_mark_strip* strips, size_t s,
                           const size_t* numbers, size_t m) {
  rmm_readings* r = nullptr;
  CHECK(rmm_text_brought(lines, n, strips, s, numbers, m, &r, nullptr) ==
        RMM_OK);
  if (!r) exit(1);
  return r;
}

int main(void) {
  rmm_mark_strip strips[] = {{B(.1, .9, .1, .2), B(0, 1, 0, .4)},
                             {B(.1, .9, .5, .6), B(0, 1, .6, 1)}};
  size_t numbers[] = {3, 7};
  rmm_placed_text reaching = W("вторая", 0, 1, .35, .95),
                  between = W("ничья", 0, 1, .45, .55);
  rmm_line lines[] = {{&reaching, 1}, {&between, 1}};
  rmm_readings* r = bring(lines, 2, strips, 2, numbers, 2);
  CHECK(r->count == 1);
  if (r->count) {
    CHECK(r->items[0].band == 7);
    CHECK(r->items[0].line_count == 1);
    rmm_placed_text p = r->items[0].lines[0].pieces[0];
    TEXT(p.text, "вторая");
    CHECK(fabs(p.box.y0 - .4375) < 1e-12 && fabs(p.box.x1 - .9) < 1e-12);
    CHECK(p.text.bytes != reaching.text.bytes);
  }
  rmm_readings_free(r);
  strips[0].in_sheet.y1 = .5;
  strips[1].in_sheet.y0 = .5;
  rmm_placed_text words[] = {W("то", .1, .4, .40, .48),
                             W("ру", .5, .8, .44, .72)};
  rmm_line line = LINE(words);
  r = bring(&line, 1, strips, 2, numbers, 2);
  CHECK(r->count == 1);
  if (r->count) {
    CHECK(r->items[0].band == 7);
    CHECK(r->items[0].lines[0].count == 2);
    TEXT(r->items[0].lines[0].pieces[1].text, "ру");
  }
  rmm_readings_free(r);
  r = bring(&line, 1, strips, 2, numbers, 1);
  CHECK(!r->count);
  rmm_readings_free(r);
  r = bring(&line, 1, strips, 0, numbers, 2);
  CHECK(!r->count);
  rmm_readings_free(r);
  rmm_placed_text boundary = W("boundary", .1, .4, .49, .51);
  line = (rmm_line){&boundary, 1};
  r = bring(&line, 1, strips, 2, numbers, 2);
  CHECK(r->count == 1);
  if (r->count) CHECK(r->items[0].band == 3);
  rmm_readings_free(r);
  /* Multiple strips may map to one band, retaining both input lines. */
  size_t repeated[] = {3, 3};
  rmm_placed_text p[] = {W("a", 0, 1, .1, .2), W("b", 0, 1, .7, .8)};
  rmm_line rows[] = {{p, 1}, {p + 1, 1}};
  r = bring(rows, 2, strips, 2, repeated, 2);
  CHECK(r->count == 1);
  if (r->count) {
    CHECK(r->items[0].line_count == 2);
    TEXT(r->items[0].lines[1].pieces[0].text, "b");
  }
  rmm_readings_free(r);
  return rm_test_report("brought");
}
