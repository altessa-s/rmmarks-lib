# RMMarks (C23)

Geometry for marked text: where a highlight lies on a page, which words it
covers, and how OCR results from a stack of cropped strips return to the page.

An independent package with no runtime dependency beyond the C standard
library. It does not depend on RMLines, RMRender, Swift, a graphics backend or
an OCR engine. The same rules are available for Swift as
[rmmarks-swift](https://github.com/altessa-s/rmmarks-swift); the build and
ownership conventions follow [rmlines-lib](https://github.com/altessa-s/rmlines-lib).

Part of **[Slate Drive](https://slatedrive.app)**
([repository](https://github.com/altessa-s/slatedrive)), the macOS app that
shows a reMarkable library in Finder.

## Build and install

CMake 3.21 or newer and a C23 compiler are required. The headers use
`constexpr`, fixed-underlying-type enums and `[[nodiscard]]`; use clang 19+
or GCC 13+ (including a sufficiently recent Apple Clang). Windows uses clang
with the MSVC SDK, not MSVC's C compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRMMARKS_WERROR=ON
cmake --build build --parallel
cmake --build build --target check
cmake --install build --prefix /your/prefix
```

| Option | Default | Purpose |
|---|---|---|
| `BUILD_SHARED_LIBS` | `OFF` | Build a shared library instead of a static library |
| `RMMARKS_BUILD_TESTS` | on for a top-level build | Build tests and the `check` target |
| `RMMARKS_WERROR` | `OFF` | Treat library compiler warnings as errors |
| `RMMARKS_BUILD_FUZZERS` | `OFF` | Build `fuzz_marks`; requires tests and clang's libFuzzer runtime |

Installation includes headers, the library, license notices, a relocatable
CMake package and pkg-config metadata. For CMake consumers:

```cmake
find_package(RMMarks 1 REQUIRED)
target_link_libraries(your_target PRIVATE RMMarks::rmmarks)
```

The target propagates the C23 requirement. For another build system:

```sh
cc -std=c2x app.c $(pkg-config --cflags --libs rmmarks) -o app
```

For static linking use `pkg-config --static --libs rmmarks` to include the
platform's math library where needed. `add_subdirectory` is supported;
tests default to off when embedded. Public names use `rmm_`/`RMM_` so they
can coexist with RMLines' `rm_`/`RM_` names without header or symbol collisions.

## Find the highlighted text

```c
#include <rmmarks/rmmarks.h>

rmm_page_box band = { .x0 = .1, .y0 = .2, .x1 = .5, .y1 = .24 };
rmm_placed_text words[] = {
    { .text = RMM_STR("marked"), .box = { .1, .2, .3, .24 } },
    { .text = RMM_STR("words"),  .box = { .31, .2, .5, .24 } },
};
rmm_line line = { .pieces = words, .count = 2 };
rmm_passages *result = nullptr;
rmm_error error;
if (rmm_text_passages(&band, 1, &line, 1, nullptr, 0, &result, &error) == RMM_OK) {
    /* result->items[0].text is "marked words"; result->items[0].band is 0. */
    rmm_passages_free(result);
}
```

Coordinates are fractions of the page, with the origin at the top left.
They are not clipped to `[0, 1]`. Pass each line as its words when available;
otherwise `rmm_text_passages_from_reading` treats each recognized line as a
single piece.

The matching rules:

- The band must cover the line's middle, with tolerance of 5% of line height.
- Covering at least 85% of the line width selects the whole line.
- Otherwise, words covered by at least half their width are selected.
- If none qualify, words touched by at least 20% of the narrower span are selected.
- Selected pieces are joined in input reading order with one ASCII space
  between them, then whitespace is trimmed at the passage's ends.
- Repeated selections of the same page-piece identities become one passage.
  Later band indexes are retained in `again`; equal text elsewhere is not a repeat.
- Results are ordered by `(place.y0, place.x0, band)`.

## A second reading and cropped strips

`rmm_band_reading` supplies an optional OCR reading for one band. The page's
reading is preferred; a differing cut reading is retained in `second`.
When only the cut found words, those words become the passage. Canonically
equivalent Unicode strings count as agreement. Cut-only passages are not
deduplicated because their piece indexes are local to their own cuts.

`rmm_text_brought` maps OCR from a stacked picture through `rmm_mark_strip`
coordinates. A line belongs to the first numbered strip containing the
middle of the *whole* line. Strips and band numbers are zipped: unmatched
trailing entries are ignored. Multiple strips may contribute to one band.
The result's `items`/`count` can be passed directly as the `alone` inputs of
`rmm_text_passages`.

`rmm_text_blocks` groups neighboring passages for cropping. It keeps joining
all reached blocks until no more can join, using a gap of 75% of the incoming
passage's height and horizontal overlap. The gap does not grow with the block.

## Ownership and failures

All inputs are borrowed for the duration of the call. Each allocated result
owns copies of every returned string and array in its own arena. Release it
once with `rmm_passages_free`, `rmm_blocks_free` or `rmm_readings_free`.
Each accepts `nullptr`. Do not free individual fields or change `arena`.

On failure, `*out` is unchanged and partial allocations are released. An
empty successful result is distinct from failure. `out_error` is optional.

| Status | Meaning |
|---|---|
| `RMM_OK` | A complete result, possibly empty |
| `RMM_ERR_INVALID_ARGUMENT` | Invalid pointer/count pair, impossible count, non-finite geometry, duplicate `alone` keys, or coordinate transform overflow |
| `RMM_ERR_OUT_OF_MEMORY` | Allocation failed or requested storage would overflow |
| `RMM_ERR_INVALID_UTF8` | An input text is not valid UTF-8 |

Pointers must address the declared counts; the API cannot validate arbitrary
addresses. Coordinates must have finite extents and finite vertical sums.
Finite degenerate and inverted boxes retain the Swift arithmetic; `inside`
returns its input unchanged for a frame with non-positive width or height.
The two direct box functions perform arithmetic without allocating or reporting
errors; callers using their outputs must check them when geometry is untrusted.

Strings are counted UTF-8. Input strings need not end in NUL; outputs do,
but may also contain embedded NULs, so use `length`. `RMM_STR` is for string
literals. Optional strings (`item`, `page_id`, `second`) use `bytes == nullptr`
for absence. An empty present string can use a non-null pointer with length zero.

The library shares no mutable production state and emits no logs. Each call
can run independently on a caller's thread. Serialization belongs to the host:
Swift's `Codable`, `Hashable` and `Sendable` conformances are not JSON codecs,
hash containers or concurrency machinery in this C package. Preserve absent
optional fields when adapting existing stored records.

## Validation

`check` runs seven suites: model, passages, blocks, brought, allocation/input
errors, Unicode and a deterministic fuzz smoke test. Allocation failure is injected at every logical
allocation in each public allocating operation; production builds do not
export the injection hooks.

CI covers static/shared builds on Linux and macOS, clang builds on Windows,
sanitizers, exact shared-library exports, installation, CMake/pkg-config
consumers, and libFuzzer. CodeQL and nightly fuzz workflows follow the sibling
package. The version is declared once in `include/rmmarks/rmmarks.h`; the release
workflow requires the pushed tag to match it.

On macOS, an optional cross-check builds the rmmarks-swift sources and
compares the two implementations on all three operations:

```sh
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON
cmake --build build-shared
python3 tests/compare_swift.py --library build-shared/librmmarks.dylib \
    --swift-sources ../rmmarks-swift/Sources/RMMarks
```

This check needs the separate Swift checkout; ordinary builds and tests do not.

## See also

- **[rmmarks-swift](https://github.com/altessa-s/rmmarks-swift)** — the same
  rules as a Swift package.
- **[rmlines-lib](https://github.com/altessa-s/rmlines-lib)** — the `.rm` v6
  page format as a C23 library; its highlights are what this library places.
- **[rmlines-swift](https://github.com/altessa-s/rmlines-swift)** and
  **[rmlines-go](https://github.com/altessa-s/rmlines-go)** — the same page
  format for Swift and Go.
- **[rmdoc-swift](https://github.com/altessa-s/rmdoc-swift)** — the `.rmdoc`
  container a document and its pages travel in.
- **[Slate Drive](https://slatedrive.app)**
  ([repository](https://github.com/altessa-s/slatedrive)) — the app these
  packages are part of.

## Acknowledgements

reMarkable does not publish its file formats. This library is built on openly
available information and on the research of third-party developers who
documented them.

## License

MIT, as in the sibling packages; see [LICENSE](LICENSE). Generated Unicode
tables carry the [Unicode data license](docs/UNICODE-LICENSE.txt).

reMarkable is a trademark of reMarkable AS. This project is not affiliated
with, endorsed by or sponsored by reMarkable AS.
