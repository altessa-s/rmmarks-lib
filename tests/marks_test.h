/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */

#ifndef MARKS_TEST_H
#define MARKS_TEST_H
#include "rmm_test.h"
#include "rmmarks/rmmarks.h"
#include <math.h>
#define B(x0, x1, y0, y1) ((rmm_page_box){x0, y0, x1, y1})
#define W(text, x0, x1, y0, y1) ((rmm_placed_text){RMM_STR(text), B(x0, x1, y0, y1)})
#define N(array) (sizeof(array) / sizeof *(array))
#define LINE(array) ((rmm_line){array, N(array)})
#define CHECK(x) RM_CHECK(x, "%s", #x)
#define TEXT(s, expected) RM_CHECK_EQ_STR((s).bytes, (s).length, expected, "text")
[[maybe_unused]] static bool box_equal(rmm_page_box a, rmm_page_box b) {
    return fabs(a.x0 - b.x0) < 1e-12 && fabs(a.y0 - b.y0) < 1e-12 && fabs(a.x1 - b.x1) < 1e-12 &&
           fabs(a.y1 - b.y1) < 1e-12;
}
[[maybe_unused]] static rmm_passages *find(const rmm_page_box *bands, size_t n,
                                           const rmm_line *lines, size_t m,
                                           const rmm_band_reading *alone, size_t a) {
    rmm_passages *r = nullptr;
    rmm_error error;
    rmm_status status = rmm_text_passages(bands, n, lines, m, alone, a, &r, &error);
    CHECK(status == RMM_OK);
    if (!r) {
        fprintf(stderr, "%s\n", error.message);
        exit(1);
    }
    return r;
}
#endif
