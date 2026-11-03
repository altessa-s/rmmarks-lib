/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */

#include "marks_test.h"

int main(void) {
  rmm_page_box frame = B(.25, .75, .25, .75), cut = B(0, .5, 0, 1),
               page = B(.25, .5, .25, .75);
  CHECK(box_equal(rmm_box_within(cut, frame), page));
  CHECK(box_equal(rmm_box_inside(page, frame), cut));
  frame = B(.1, .8, .2, .6);
  page = B(.3, .5, .3, .4);
  CHECK(box_equal(rmm_box_within(rmm_box_inside(page, frame), frame), page));
  rmm_page_box flat = B(.5, .5, .5, .5);
  CHECK(box_equal(rmm_box_inside(page, flat), page));
  CHECK(box_equal(rmm_box_within(page, flat), flat));
  CHECK(box_equal(rmm_box_inside(page, B(.8, .2, 0, 1)), page));
  CHECK(rmm_box_within(B(-1, 2, -1, 2), frame).x0 < 0);
  rmm_marked_page p = {0};
  CHECK(rmm_page_is_empty(&p));
  CHECK(rmm_page_is_empty(nullptr));
  rmm_mark_band band = {.box = page, .color = 0xFEFD60};
  CHECK(band.item.bytes == nullptr);
  p.bands = &band;
  p.band_count = 1;
  CHECK(!rmm_page_is_empty(&p));
  rmm_marked_highlight h = {.text = RMM_STR("слово"), .band = band};
  p = (rmm_marked_page){.highlights = &h, .highlight_count = 1};
  CHECK(!rmm_page_is_empty(&p));
  rmm_recognized_line recognized = {RMM_STR("Здесь отмечено одно"), page};
  rmm_placed_text placed = rmm_place_line(recognized);
  TEXT(placed.text, "Здесь отмечено одно");
  CHECK(box_equal(placed.box, page));
  CHECK(strcmp(RMMARKS_VERSION_STRING, "1.0.1") == 0);
  for (int i = 0; i <= 3; i += 1)
    CHECK(strlen(rmm_status_string((rmm_status)i)) > 0);
  CHECK(strcmp(rmm_status_string((rmm_status)999), "unknown status") == 0);
  rmm_passages_free(nullptr);
  rmm_blocks_free(nullptr);
  rmm_readings_free(nullptr);
  return rm_test_report("model");
}
