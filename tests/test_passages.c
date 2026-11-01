/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */

#include "marks_test.h"

static void handwritten_notes(void) {
    rmm_placed_text a[] = {
        W("потом", .131, .208, .295, .327), W("ещё", .211, .278, .295, .327),
        W("нужно", .281, .410, .295, .327), W("проверить", .413, .673, .295, .327),
        W("отчёт", .676, .829, .295, .327), W("у", .832, .863, .295, .327)};
    rmm_placed_text b[] = {W("открыть", .142, .284, .475, .515),
                           W("первый", .289, .487, .475, .515), W("раздел", .491, .625, .475, .515),
                           W("для", .629, .703, .475, .515), W("записей", .708, .906, .475, .515)};
    rmm_placed_text c[] = {W("таблицами", .158, .442, .511, .528), W("и", .445, .485, .511, .528),
                           W("графиками", .488, .688, .511, .528)};
    rmm_line lines[] = {LINE(a), LINE(b), LINE(c)};
    rmm_page_box bands[] = {B(.464, .700, .508, .530), B(.119, .880, .295, .319),
                            B(.129, .289, .481, .502)};
    rmm_passages *r = find(bands, 3, lines, 3, nullptr, 0);
    CHECK(r->count == 3);
    if (r->count == 3) {
        TEXT(r->items[0].text, "потом ещё нужно проверить отчёт у");
        CHECK(r->items[0].band == 1);
        TEXT(r->items[1].text, "открыть");
        CHECK(r->items[1].band == 2);
        TEXT(r->items[2].text, "и графиками");
        CHECK(r->items[2].band == 0);
        CHECK(!r->items[0].again_count);
        CHECK(!r->items[0].second.bytes);
    }
    rmm_passages_free(r);
    rmm_page_box repeated[] = {bands[1], B(.129, .870, .297, .319), bands[0], bands[1]};
    r = find(repeated, 4, lines, 3, nullptr, 0);
    CHECK(r->count == 2);
    if (r->count == 2) {
        CHECK(r->items[0].band == 0);
        CHECK(r->items[0].again_count == 2);
        CHECK(r->items[0].again[0] == 1 && r->items[0].again[1] == 3);
    }
    rmm_passages_free(r);
    rmm_page_box blank = B(.1, .5, .8, .84);
    r = find(&blank, 1, lines, 3, nullptr, 0);
    CHECK(!r->count);
    rmm_passages_free(r);
}
static void word_and_line(void) {
    rmm_placed_text words[] = {
        W("Здесь", .152, .176, .259, .298), W("отмечено", .181, .287, .259, .298),
        W("одно", .292, .461, .259, .298), W("последнее", .466, .616, .259, .298),
        W("слово.", .621, .756, .259, .298)};
    rmm_line line = LINE(words);
    rmm_page_box band = B(.629, .755, .270, .286);
    rmm_passages *r = find(&band, 1, &line, 1, nullptr, 0);
    CHECK(r->count == 1);
    if (r->count)
        TEXT(r->items[0].text, "слово.");
    rmm_passages_free(r);
    rmm_recognized_line reading = {RMM_STR("Здесь отмечено одно последнее слово."),
                                   B(.152, .756, .259, .298)};
    r = nullptr;
    CHECK(rmm_text_passages_from_reading(&band, 1, &reading, 1, &r, nullptr) == RMM_OK);
    CHECK(r && r->count == 1);
    if (r && r->count)
        TEXT(r->items[0].text, "Здесь отмечено одно последнее слово.");
    rmm_passages_free(r);
}
static void columns_and_split_lines(void) {
    rmm_placed_text left = W("OPŠTE ODREDBE UGOVORA O SARADNJI", .090, .472, .446, .465);
    rmm_placed_text first = W("ОБЩИЕ ПОЛОЖЕНИЯ", .536, .873, .448, .465);
    rmm_placed_text second = W("РАЗДЕЛА О ПОРЯДКЕ РАБОТЫ", .541, .871, .467, .483);
    rmm_line lines[] = {{&left, 1}, {&first, 1}, {&second, 1}};
    rmm_page_box bands[] = {B(.535, .899, .442, .465), B(.524, .896, .458, .482)};
    rmm_passages *r = find(bands, 2, lines, 3, nullptr, 0);
    CHECK(r->count == 2);
    if (r->count == 2) {
        TEXT(r->items[0].text, "ОБЩИЕ ПОЛОЖЕНИЯ");
        TEXT(r->items[1].text, "РАЗДЕЛА О ПОРЯДКЕ РАБОТЫ");
    }
    rmm_passages_free(r);
    rmm_placed_text pieces[] = {W("Strane su saglasne da je ovaj tekst", .077, .489, .557, .576),
                                W("Первая", .497, .601, .561, .576),
                                W("строка", .664, .735, .562, .573),
                                W("раздела", .797, .914, .562, .571)};
    rmm_line observations[4];
    for (size_t i = 0; i < 4; i += 1)
        observations[i] = (rmm_line){pieces + i, 1};
    rmm_page_box band = B(.499, .916, .557, .581);
    r = find(&band, 1, observations, 4, nullptr, 0);
    CHECK(r->count == 1);
    if (r->count) {
        TEXT(r->items[0].text, "Первая строка раздела");
        CHECK(box_equal(r->items[0].place, B(.497, .914, .561, .576)));
    }
    rmm_passages_free(r);
}
static void second_reading(void) {
    rmm_page_box band = B(.1, .9, .29, .33);
    rmm_placed_text main = W("потом ещё нужно проверить отчёт и", .13, .86, .295, .327);
    rmm_placed_text other = W("потом ещё нужно проворить отчёт и", .13, .86, .295, .327);
    rmm_line page = {&main, 1}, cut = {&other, 1};
    rmm_band_reading alone = {0, &cut, 1};
    rmm_passages *r = find(&band, 1, &page, 1, &alone, 1);
    CHECK(r->count == 1);
    if (r->count) {
        TEXT(r->items[0].text, "потом ещё нужно проверить отчёт и");
        TEXT(r->items[0].second, "потом ещё нужно проворить отчёт и");
    }
    rmm_passages_free(r);
    r = find(&band, 1, nullptr, 0, &alone, 1);
    CHECK(r->count == 1);
    if (r->count) {
        TEXT(r->items[0].text, "потом ещё нужно проворить отчёт и");
        CHECK(!r->items[0].second.bytes);
    }
    rmm_passages_free(r);
    alone.lines = &page;
    r = find(&band, 1, &page, 1, &alone, 1);
    CHECK(r->count == 1);
    if (r->count)
        CHECK(!r->items[0].second.bytes);
    rmm_passages_free(r);
    alone.line_count = 0;
    r = find(&band, 1, nullptr, 0, &alone, 1);
    CHECK(!r->count);
    rmm_passages_free(r);
    rmm_page_box twice[] = {band, band};
    rmm_band_reading cuts[] = {{0, &cut, 1}, {1, &cut, 1}};
    r = find(twice, 2, nullptr, 0, cuts, 2);
    CHECK(r->count == 2);
    rmm_passages_free(r);
    /* Same spelling in different locations is not a duplicate. */
    rmm_placed_text duplicate[] = {W("word", .1, .4, .1, .2), W("word", .1, .4, .5, .6)};
    rmm_line rows[] = {{duplicate, 1}, {duplicate + 1, 1}};
    rmm_page_box marks[] = {B(0, 1, .1, .2), B(0, 1, .5, .6)};
    r = find(marks, 2, rows, 2, nullptr, 0);
    CHECK(r->count == 2);
    rmm_passages_free(r);
}
static void thresholds_and_ownership(void) {
    char source[] = "word";
    rmm_placed_text words[] = {{{source, 4}, B(0, 1, 0, 1)}, W("tail", 1.1, 2, 0, 1)};
    rmm_line line = LINE(words);
    rmm_page_box band = B(0, .5, .49, .51);
    rmm_passages *r = find(&band, 1, &line, 1, nullptr, 0);
    CHECK(r->count == 1);
    source[0] = 'X';
    if (r->count) {
        TEXT(r->items[0].text, "word");
        CHECK(r->items[0].text.bytes[4] == '\0');
    }
    rmm_passages_free(r);
    band = B(.2, .3, .49, .51);
    r = find(&band, 1, &line, 1, nullptr, 0);
    CHECK(r->count == 1);
    if (r->count)
        TEXT(r->items[0].text, "Xord");
    rmm_passages_free(r);
    band = B(5, 6, 0, 1);
    r = find(&band, 1, &line, 1, nullptr, 0);
    CHECK(!r->count);
    rmm_passages_free(r);
    rmm_placed_text spaced[] = {W("  ", .1, .2, .3, .34), W("слово", .21, .4, .3, .34),
                                W(" ", .41, .5, .3, .34)};
    line = LINE(spaced);
    band = B(.09, .51, .3, .34);
    r = find(&band, 1, &line, 1, nullptr, 0);
    CHECK(r->count == 1);
    if (r->count)
        TEXT(r->items[0].text, "слово");
    rmm_passages_free(r);
    spaced[1].text = RMM_STR(" ");
    r = find(&band, 1, &line, 1, nullptr, 0);
    CHECK(!r->count);
    rmm_passages_free(r);
}
int main(void) {
    handwritten_notes();
    word_and_line();
    columns_and_split_lines();
    second_reading();
    thresholds_and_ownership();
    return rm_test_report("passages");
}
