// Copyright 2026 ALTESSA SOLUTIONS INC.
// Use of this source code is governed by an MIT-style license that can be
// found in the LICENSE file.

// Runs the rmmarks-swift sources on the cross-check inputs.
import Foundation

struct Word: Decodable {
    let text: String
    let box: PageBox
    var placed: PlacedText { PlacedText(text: text, box: box) }
}
struct Reading: Decodable { let band: Int; let lines: [[Word]] }
struct Strip: Decodable { let onPage: PageBox; let inSheet: PageBox }
struct Input: Decodable {
    let bands: [PageBox]
    let lines: [[Word]]
    let alone: [Reading]
    let strips: [Strip]
    let numbers: [Int]
    let blocks: [PageBox]
}
func box(_ b: PageBox) -> [Double] { [b.x0, b.y0, b.x1, b.y1] }
let inputs = try JSONDecoder().decode([Input].self, from: FileHandle.standardInput.readDataToEndOfFile())
let outputs: [[String: Any]] = inputs.map { input in
    let lines = input.lines.map { $0.map(\.placed) }
    let alone = Dictionary(uniqueKeysWithValues: input.alone.map { ($0.band, $0.lines.map { $0.map(\.placed) }) })
    let passages = MarkedText.passages(bands: input.bands, lines: lines, alone: alone)
    let blocks = MarkedText.blocks(of: input.blocks.enumerated().map {
        MarkedPassage(text: "", band: $0.offset, place: $0.element)
    })
    let brought = MarkedText.brought(lines, through: input.strips.map {
        MarkStrip(onPage: $0.onPage, inSheet: $0.inSheet)
    }, under: input.numbers)
    return [
        "passages": passages.map { p -> [String: Any] in
            ["text": p.text, "band": p.band, "place": box(p.place),
             "second": p.second.map { $0 as Any } ?? NSNull(), "again": p.again]
        },
        "blocks": blocks.map(box),
        "brought": brought.keys.sorted().map { key -> [String: Any] in
            ["band": key, "lines": brought[key]!.map { row in
                row.map { ["text": $0.text, "box": box($0.box)] as [String: Any] }
            }]
        },
    ]
}
FileHandle.standardOutput.write(try JSONSerialization.data(withJSONObject: outputs, options: [.sortedKeys]))
