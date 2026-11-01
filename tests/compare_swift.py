#!/usr/bin/env python3
# Copyright 2026 ALTESSA SOLUTIONS INC.
# Use of this source code is governed by an MIT-style license that can be
# found in the LICENSE file.
"""Cross-check against an rmmarks-swift checkout (macOS).

python3 tests/compare_swift.py --library build-shared/librmmarks.dylib \
    --swift-sources ../rmmarks-swift/Sources/RMMarks
Python's standard library and a Swift compiler are sufficient.
"""
import argparse
import ctypes as c
import json
import math
from pathlib import Path
import random
import subprocess
import tempfile


class Str(c.Structure):
    _fields_ = [('bytes', c.c_void_p), ('length', c.c_size_t)]


class Box(c.Structure):
    _fields_ = [(key, c.c_double) for key in ('x0', 'y0', 'x1', 'y1')]


class Word(c.Structure):
    _fields_ = [('text', Str), ('box', Box)]


class Line(c.Structure):
    _fields_ = [('pieces', c.POINTER(Word)), ('count', c.c_size_t)]


class Reading(c.Structure):
    _fields_ = [('band', c.c_size_t), ('lines', c.POINTER(Line)), ('line_count', c.c_size_t)]


class Passage(c.Structure):
    _fields_ = [('text', Str), ('band', c.c_size_t), ('place', Box), ('second', Str),
                ('again', c.POINTER(c.c_size_t)), ('again_count', c.c_size_t)]


class Strip(c.Structure):
    _fields_ = [('on_page', Box), ('in_sheet', Box)]


def result_type(item):
    class Result(c.Structure):
        _fields_ = [('items', c.POINTER(item)), ('count', c.c_size_t), ('arena', c.c_void_p)]
    return Result


Passages, Blocks, Readings = map(result_type, (Passage, Box, Reading))


def box(values):
    return Box(*(values[key] for key in ('x0', 'y0', 'x1', 'y1')))


def box_list(value):
    return [getattr(value, key) for key in ('x0', 'y0', 'x1', 'y1')]


def string(value):
    return c.string_at(value.bytes, value.length).decode('utf-8') if value.bytes else None


def run_c(lib, case):
    keep = []

    def lines(rows):
        result = (Line * len(rows))()
        for i, row in enumerate(rows):
            words = (Word * len(row))()
            keep.append(words)
            for j, word in enumerate(row):
                data = word['text'].encode('utf-8')
                buffer = c.create_string_buffer(data)
                keep.append(buffer)
                words[j] = Word(Str(c.cast(buffer, c.c_void_p), len(data)), box(word['box']))
            result[i] = Line(words, len(row))
        keep.append(result)
        return result

    bands = (Box * len(case['bands']))(*(box(b) for b in case['bands']))
    rows = lines(case['lines'])
    alone = (Reading * len(case['alone']))()
    for i, own in enumerate(case['alone']):
        alone[i] = Reading(own['band'], lines(own['lines']), len(own['lines']))
    found = c.POINTER(Passages)()
    assert lib.rmm_text_passages(bands, len(bands), rows, len(rows), alone, len(alone), c.byref(found), None) == 0
    try:
        passages = []
        for p in found.contents.items[:found.contents.count]:
            passages.append(dict(text=string(p.text), band=p.band, place=box_list(p.place),
                                 second=string(p.second), again=list(p.again[:p.again_count])))
    finally:
        lib.rmm_passages_free(found)
    placed = (Passage * len(case['blocks']))()
    for i, b in enumerate(case['blocks']):
        placed[i].place = box(b)
    merged = c.POINTER(Blocks)()
    assert lib.rmm_text_blocks(placed, len(placed), c.byref(merged), None) == 0
    try:
        blocks = [box_list(b) for b in merged.contents.items[:merged.contents.count]]
    finally:
        lib.rmm_blocks_free(merged)
    strips = (Strip * len(case['strips']))(*(Strip(box(s['onPage']), box(s['inSheet'])) for s in case['strips']))
    numbers = (c.c_size_t * len(case['numbers']))(*case['numbers'])
    brought = c.POINTER(Readings)()
    assert lib.rmm_text_brought(rows, len(rows), strips, len(strips), numbers, len(numbers), c.byref(brought), None) == 0
    try:
        readings = []
        for group in brought.contents.items[:brought.contents.count]:
            readings.append(dict(band=group.band, lines=[[
                dict(text=string(p.text), box=box_list(p.box)) for p in row.pieces[:row.count]
            ] for row in group.lines[:group.line_count]]))
    finally:
        lib.rmm_readings_free(brought)
    return dict(passages=passages, blocks=blocks, brought=sorted(readings, key=lambda r: r['band']))


def same(a, b):
    if isinstance(a, float) or isinstance(b, float):
        return isinstance(a, (float, int)) and isinstance(b, (float, int)) and math.isclose(a, b, abs_tol=1e-12, rel_tol=1e-12)
    if isinstance(a, dict):
        return isinstance(b, dict) and a.keys() == b.keys() and all(same(a[k], b[k]) for k in a)
    if isinstance(a, list):
        return isinstance(b, list) and len(a) == len(b) and all(same(x, y) for x, y in zip(a, b))
    return a == b


def cases(count):
    rng = random.Random(0x524D4D41524B53)
    words = ['', ' ', '\t word\n', 'слово', 'العربية', '😀', 'café', 'cafe\u0301',
             '\u00a0 word\u3000', ' \u0301word \u0301', '\u0600 word', '\n\u0301word', 'a\0b']

    def rectangle(inverted=False):
        x, y = rng.randrange(-5, 20)/20, rng.randrange(-5, 20)/20
        w, h = rng.randrange(0, 20)/20, rng.randrange(0, 10)/20
        return dict(x0=x, y0=y, x1=x-w if inverted else x+w, y1=y+h)

    def rows():
        return [[dict(text=rng.choice(words), box=rectangle()) for _ in range(rng.randrange(0, 5))]
                for _ in range(rng.randrange(0, 8))]

    for _ in range(count):
        bands = [rectangle(rng.random() < .05) for _ in range(rng.randrange(0, 8))]
        if bands and rng.random() < .5:
            bands += [rng.choice(bands)]
        yield dict(bands=bands, lines=rows(), alone=[dict(band=i, lines=rows()) for i in range(len(bands)) if rng.random() < .5],
                   strips=[dict(onPage=rectangle(), inSheet=rectangle()) for _ in range(rng.randrange(0, 5))],
                   numbers=[rng.randrange(0, 8) for _ in range(rng.randrange(0, 5))],
                   blocks=[rectangle() for _ in range(rng.randrange(0, 12))])
    for a, b in [('café', 'cafe\u0301'), ('й', 'и\u0306'), ('각', '\u1100\u1161\u11a8'),
                 ('x\u0301\u0323', 'x\u0323\u0301'), ('Ａ', 'A')]:
        full = dict(x0=0, y0=0, x1=1, y1=1)
        yield dict(bands=[full], lines=[[dict(text=a, box=full)]],
                   alone=[dict(band=0, lines=[[dict(text=b, box=full)]])], strips=[], numbers=[], blocks=[])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--swift-sources', type=Path, required=True)
    parser.add_argument('--count', type=int, default=2000)
    args = parser.parse_args()
    lib = c.CDLL(str(args.library.resolve()))
    lib.rmm_text_passages.argtypes = [c.POINTER(Box), c.c_size_t, c.POINTER(Line), c.c_size_t,
                                     c.POINTER(Reading), c.c_size_t, c.POINTER(c.POINTER(Passages)), c.c_void_p]
    lib.rmm_text_blocks.argtypes = [c.POINTER(Passage), c.c_size_t, c.POINTER(c.POINTER(Blocks)), c.c_void_p]
    lib.rmm_text_brought.argtypes = [c.POINTER(Line), c.c_size_t, c.POINTER(Strip), c.c_size_t,
                                    c.POINTER(c.c_size_t), c.c_size_t, c.POINTER(c.POINTER(Readings)), c.c_void_p]
    for name, kind in [('passages', Passages), ('blocks', Blocks), ('readings', Readings)]:
        function = getattr(lib, 'rmm_' + name + '_free')
        function.argtypes = [c.POINTER(kind)]
        function.restype = None
    inputs = list(cases(args.count))
    with tempfile.TemporaryDirectory(prefix='rmmarks-crosscheck-') as directory:
        executable = Path(directory) / 'swift'
        subprocess.run(['swiftc', '-module-cache-path', directory + '/cache', '-o', str(executable),
                        *map(str, sorted(args.swift_sources.glob('*.swift'))),
                        str(Path(__file__).parent / 'swift-crosscheck/main.swift')], check=True)
        output = subprocess.run([str(executable)], input=json.dumps(inputs).encode(), stdout=subprocess.PIPE, check=True)
        expected = json.loads(output.stdout)
    for i, (case, reference) in enumerate(zip(inputs, expected)):
        actual = run_c(lib, case)
        if not same(actual, reference):
            print(json.dumps(dict(case=i, input=case, swift=reference, c=actual), ensure_ascii=False, indent=2))
            raise SystemExit('Swift/C mismatch')
    print(f'{len(inputs)} cases agree with Swift for passages, blocks and brought')


if __name__ == '__main__':
    main()
