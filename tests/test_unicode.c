/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */

#include "marks_test.h"
#include "rmm_internal.h"

static void trimmed(const char *input, const char *expected) {
    rmm_placed_text p = {{input, strlen(input)}, B(0, 1, 0, 1)};
    rmm_line line = {&p, 1};
    rmm_page_box band = B(0, 1, 0, 1);
    rmm_passages *r = find(&band, 1, &line, 1, nullptr, 0);
    if (*expected) {
        CHECK(r->count == 1);
        if (r->count)
            TEXT(r->items[0].text, expected);
    } else
        CHECK(r->count == 0);
    rmm_passages_free(r);
}
static void equal_readings(const char *a, const char *b, bool equal) {
    rmm_page_box band = B(0, 1, 0, 1);
    rmm_placed_text p = {{a, strlen(a)}, band}, q = {{b, strlen(b)}, band};
    rmm_line page = {&p, 1}, cut = {&q, 1};
    rmm_band_reading alone = {0, &cut, 1};
    rmm_passages *r = find(&band, 1, &page, 1, &alone, 1);
    CHECK(r->count == 1);
    if (r->count)
        CHECK((r->items[0].second.bytes == nullptr) == equal);
    rmm_passages_free(r);
}
int main(void) {
    trimmed("\t\r\n слово \t", "слово");
    trimmed("\u00A0\u1680\u2003слово\u202F\u205F\u3000", "слово");
    trimmed(" \u0301слово \u0301", "слово");
    trimmed(" \u200Dслово \u200D", "слово");
    /* U+0085 as its UTF-8 bytes: C allows no universal character name below
     * U+00A0, and GCC refuses one. */
    trimmed("\xC2\x85" "\u2028\u2029", "");
    trimmed("\u200Bword\u200B", "\u200Bword\u200B");
    trimmed("\u0600 word", "\u0600 word");
    trimmed("\n\u0301word\n\u0301", "\u0301word\n\u0301");
    equal_readings("café", "cafe\u0301", true);
    equal_readings("й", "и\u0306", true);
    equal_readings("각", "\u1100\u1161\u11A8", true);
    equal_readings("x\u0301\u0323", "x\u0323\u0301", true);
    equal_readings("café", "cafe", false);
    equal_readings("Ａ", "A", false); /* Canonical, not compatibility equality. */
    equal_readings("Word", "word", false);
    const unsigned char bad[][4] = {{0xC0, 0x80, 0, 0},       {0xED, 0xA0, 0x80, 0},
                                    {0xF4, 0x90, 0x80, 0x80}, {0xE2, 0x82, 0, 0},
                                    {0xFF, 0, 0, 0},          {0x80, 0, 0, 0}};
    size_t lengths[] = {2, 3, 4, 2, 1, 1};
    for (size_t i = 0; i < N(bad); i += 1)
        CHECK(!rmm_utf8_valid((rmm_str){(const char *)bad[i], lengths[i]}));
    CHECK(rmm_utf8_valid(RMM_STR("😀 Привет 你好 العربية")));
    /* Counted strings preserve embedded NUL rather than truncating at it. */
    const char with_zero[] = {'a', 0, 'b'};
    rmm_page_box band = B(0, 1, 0, 1);
    rmm_placed_text p = {{with_zero, 3}, band};
    rmm_line line = {&p, 1};
    rmm_passages *r = find(&band, 1, &line, 1, nullptr, 0);
    CHECK(r->count == 1);
    if (r->count) {
        CHECK(r->items[0].text.length == 3);
        CHECK(memcmp(r->items[0].text.bytes, with_zero, 3) == 0);
    }
    rmm_passages_free(r);
    return rm_test_report("unicode");
}
