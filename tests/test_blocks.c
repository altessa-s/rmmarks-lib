/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */

#include "marks_test.h"
static rmm_blocks* blocks(const rmm_page_box* boxes, size_t n) {
  rmm_marked_passage p[16];
  CHECK(n <= N(p));
  for (size_t i = 0; i < n; i += 1)
    p[i] = (rmm_marked_passage){.place = boxes[i]};
  rmm_blocks* r = nullptr;
  CHECK(rmm_text_blocks(p, n, &r, nullptr) == RMM_OK);
  if (!r) exit(1);
  return r;
}
int main(void) {
  rmm_page_box heading[] = {B(.536, .873, .448, .465),
                            B(.541, .871, .467, .483)};
  rmm_blocks* r = blocks(heading, 2);
  CHECK(r->count == 1);
  if (r->count) CHECK(box_equal(r->items[0], B(.536, .873, .448, .483)));
  rmm_blocks_free(r);
  rmm_page_box notes[] = {B(.142, .284, .475, .515), B(.445, .688, .511, .528)};
  r = blocks(notes, 2);
  CHECK(r->count == 2);
  rmm_blocks_free(r);
  rmm_page_box reverse[] = {B(.1, .5, .8, .83), B(.1, .5, .2, .23)};
  r = blocks(reverse, 2);
  CHECK(r->count == 2);
  if (r->count == 2) CHECK(r->items[0].y0 == .2 && r->items[1].y0 == .8);
  rmm_blocks_free(r);
  rmm_page_box bridged[] = {B(0, .3, .10, .14), B(.6, .9, .15, .19),
                            B(.2, .7, .16, .20), B(.1, .5, .70, .74)};
  r = blocks(bridged, 3);
  CHECK(r->count == 1);
  if (r->count) CHECK(box_equal(r->items[0], B(0, .9, .10, .20)));
  rmm_blocks_free(r);
  r = blocks(bridged, 4);
  CHECK(r->count == 2);
  for (size_t i = 0; i < r->count; i += 1)
    for (size_t j = i + 1; j < r->count; j += 1) {
      rmm_page_box a = r->items[i], b = r->items[j];
      CHECK(!(a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1));
    }
  rmm_blocks_free(r);
  r = blocks(nullptr, 0);
  CHECK(!r->count);
  rmm_blocks_free(r);
  /* Fixed gap from each incoming passage, not enlarged with the block. */
  rmm_page_box gap[] = {B(0, 1, 0, 1), B(0, 1, 1.75, 2.75)};
  r = blocks(gap, 2);
  CHECK(r->count == 2);
  rmm_blocks_free(r);
  return rm_test_report("blocks");
}
