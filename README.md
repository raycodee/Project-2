# GEvents — Group 75

GEvents helps students find campus events by keyword, category, date, and interests. The current code contains Raymond Shi's hash index, filters, relevance ranking, automated tests, and a small synthetic integration example. The team's planned comparison is a handwritten Trie versus a handwritten hash table. The original Merge Sort/Quick Sort proposal is superseded by the team working plan.

Team: Hayden (Khang) Luong, Raymond Shi (`raycodee`), and Phuc Nguyen. The repository is [raycodee/Project-2](https://github.com/raycodee/Project-2).

## Build and run

Requirements: a C++17 compiler (Clang, GCC, or MSVC), CMake 3.16 or newer, and a build tool supported by CMake. No third-party C++ libraries are required.

From the repository root on macOS or Linux:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/raymond_demo
```

With a multi-configuration generator such as Visual Studio:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/Release/raymond_demo.exe
```

Without CMake on macOS or Linux:

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/Text.cpp src/HashTable.cpp src/Filters.cpp src/Ranking.cpp examples/raymond_demo.cpp -o /tmp/gevents_demo
/tmp/gevents_demo
```

The example uses six fictional records built into the executable. It looks up `coding`, filters Technology events from November 5 through November 11, 2026, and ranks them for interests `coding` and `robotics`. Expected results:

```text
Posting IDs: 101 104 105 106
101 | Coding Workshop | 2026-11-05 18:00 | Demo Room A | score=13
106 | Coding Lecture | 2026-11-09 14:00 | Demo Room B | score=3
```

The demo also prints its filter settings. It is an integration example; Phuc's interactive console menu and dataset loader still need to be connected.

## Implemented components

| File | Purpose |
| --- | --- |
| `include/gevents/Event.h` | Shared event record, ID, and posting-list types |
| `include/gevents/HashTable.h`, `src/HashTable.cpp` | Handwritten hashing, linked collision chains, insertion, lookup, and resizing |
| `include/gevents/Text.h`, `src/Text.cpp` | Shared ASCII keyword and category normalization |
| `include/gevents/Filters.h`, `src/Filters.cpp` | Category OR filters, inclusive dates, and seven-day windows |
| `include/gevents/Ranking.h`, `src/Ranking.cpp` | Interest scoring and ordered recommendations |
| `tests/` | Hash-table, filter, ranking, and integration checks |
| `examples/raymond_demo.cpp` | Small reproducible pipeline example |

The index does not use `std::unordered_map`, `std::map`, or `std::set`. Bucket heads and posting lists use vectors; collision nodes and links are managed explicitly with `std::unique_ptr`. `std::sort`, `std::lower_bound`, and `std::binary_search` support posting lists and recommendation ranking. Sorting is not one of the two structures being compared in the revised plan.

## Interfaces for integration

All APIs live in namespace `gevents`; include headers as `#include "gevents/HashTable.h"` and link the `gevents` CMake target. Teammates should review this shared contract before integrating their code.

### Event and dataset contract

`Event` has fields in this order: `eventId`, `title`, `description`, `category`, `date`, `time`, `location`, `organizer`, and `tags`. `eventId` is a unique `std::uint32_t`; IDs are identifiers, not vector offsets. `tags` is `std::vector<std::string>`. Dates must be real Gregorian dates in fixed `YYYY-MM-DD` format, years 0001–9999. Times must be empty or 24-hour `HH:MM`; empty times sort before specified times on the same date. Dates and times are local campus wall-clock values; there is no timezone conversion.

Phuc's parser should convert the generator's external fields into this record. No CSV column order or tag separator has been imposed on the generator. Validate unique IDs when loading data. Events must remain unchanged after indexing; rebuild the index after changing text or removing records, since deletion/update is not provided.

### Normalization and index

`tokenize(text)` lowercases ASCII A–Z, preserves ASCII a–z and digits, and splits at every other byte. For example, `"UF's C++ Club"` becomes `uf`, `s`, `c`, `club`. There is no stemming, stop-word removal, phrase matching, or Unicode-aware tokenization. The Trie and query parser should use the same function for comparable results.

```cpp
gevents::HashTable index;
for (const auto& event : events) {
    index.indexEvent(event);
}
const gevents::PostingList* ids = index.find("coding");
```

`indexEvent` indexes title, description, category, and tags. Location, organizer, date, and time are not indexed. `insert(keyword, id)` and `find(keyword)` accept one normalized token, with surrounding punctuation allowed. Multi-token inputs throw `std::invalid_argument`; tokenize queries before lookup. Empty/punctuation-only inputs are ignored and do not produce matches. `find` returns `nullptr` for a missing token; a present posting list is always sorted by ascending ID without duplicates. `insert` returns whether a new keyword/ID pair was added.

`size()` counts distinct keywords. `bucketCount()`, `loadFactor()`, and `bucketIndex(keyword)` expose index statistics. The load factor counts keywords per bucket, not posting IDs. The table doubles its bucket count before a new keyword would exceed 0.75. FNV-1a hashing is deterministic. It is not intended for adversarial input or security applications.

The table owns its nodes and postings and is neither copyable nor movable. A pointer returned by `find` remains valid through inserts and rehashes, but not through `clear` or destruction. Iterators/references into the vector's ID elements can be invalidated when that posting list grows. `clear()` removes nodes and retains the bucket array. The class is intended for single-threaded use.

Phuc handles multi-keyword parsing and posting-list intersection. An ID lookup should use a separate agreed record-access mechanism; do not assume that an ID equals its row position. The demo resolves IDs by scanning its tiny fixture only.

### Filters

```cpp
gevents::EventFilter filters{{"technology", "social"}, "2026-11-05", "2026-11-30"};
auto candidates = gevents::filterEvents(events, filters);
auto week = gevents::weekStartingOn("2026-11-05", {"technology"});
```

Categories are ORed, with token normalization and spaces collapsed (`"Data-Science"` matches `"data science"`). Date and category conditions are ANDed. An empty category list allows all categories. An absent date bound is unbounded. Both date endpoints are inclusive. A week starts on the supplied date and includes that date plus the next six dates; it does not infer the start of a calendar week.

Filters preserve input order and return owned copies. Invalid bounds, reversed ranges, punctuation-only category selections, malformed event dates, or a week extending beyond year 9999 throw `std::invalid_argument`. Event dates are validated even when a record would not match the selected category. `isValidDate` can be used by the loader before filtering.

### Relevance ranking

```cpp
auto results = gevents::rankEvents(candidates, {"coding", "robotics"}, 10);
```

Interests are tokenized and deduplicated. Each distinct interest earns at most the following points in each field:

| Matching field | Points |
| --- | ---: |
| Any tag | 5 |
| Category | 3 |
| Title | 2 |
| Description | 1 |

A token matching every field earns 11 points. Repeating a token in a field or repeating an interest does not add points. Multiword interests are treated as separate tokens. These weights are an explicit baseline for team review, not a learned preference model.

`scoreEvent(event, interests)` scores one event's text. `rankEvents` scores every candidate, then orders by descending score, ascending date, ascending time, and ascending event ID. `RankedEvent` owns an event copy and its `relevanceScore`. A positive limit truncates after sorting; zero returns all results. Zero-score candidates remain in the results. With empty interests, chronological ordering decides the results. Ranking validates event dates and times and throws `std::invalid_argument` for malformed values. Use unique IDs for deterministic ties.

## Tests and validation

The three CTest executables contain 14 test groups:

- Hash table: empty/normalized inputs, forced collision chains, sorted duplicate-free postings and rehashing, randomized reference comparisons, event indexing, and 100,000 generated IDs.
- Filters: calendar validation, category/date bounds, seven-day windows, and invalid input handling.
- Ranking: field weights and deduplication, ordering and limits, date/time validation, and the index/filter/rank pipeline.

Tests use explicit runtime checks, so Release builds do not disable them. The large index test creates 100,000 distinct keywords plus a common keyword containing all 100,000 IDs; it does not create the team's full event dataset or measure benchmark performance.

For address and undefined-behavior checks with Clang or GCC:

```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DGEVENTS_ENABLE_SANITIZERS=ON
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure
./build-sanitize/raymond_demo
```

All project targets compile with strict warnings. Correctness checks and sanitizer runs are not performance benchmarks; Hayden's harness should measure identical normalized datasets and query workloads in optimized builds.

## Complexity

Let U be distinct keywords, B buckets, P total keyword/ID postings, p a single keyword's posting-list length, w maximum keyword length, N candidate events, C selected categories, L maximum event text length, T maximum tokens per event, and K interest tokens before deduplication. String operations depend on their length.

| Operation | Worst-case time | Notes |
| --- | --- | --- |
| Hash lookup | O(w(U + 1)) | Every keyword can collide; expected O(w(1 + U/B)) under well-distributed hashes |
| Insert one posting without resizing | O(w(U + 1) + p) | Binary search takes O(log p); vector insertion can shift p IDs |
| Resize | O(B + Uw) | Allocate a new bucket array and relink keyword nodes; posting vectors are not copied |
| Insert including a resize | O(w(U + 1) + p + B) | Bucket initialization is part of the worst-case cost |
| Clear/destruction | O(B + U + P) | Walk all chains; release all posting storage |
| Filter | O(CL + NCL + NL + S) | Category normalization/comparison plus copying S bytes of selected records; without categories, O(N + S) for valid fixed-size dates |
| Rank | O(KL + wK log(K + 1) + N(L + wT log(T + 1) + wK log(T + 1)) + N log(N + 1) + S) | Normalize interests once; tokenize/sort fields; binary-search interests; sort fixed-format ties; copy S bytes of records |

The ranking bound is deliberately conservative: interest input can contain K strings of up to L bytes each. Hash index storage is O(B + Uw + P), in addition to the source dataset. A resize needs O(B) temporary bucket storage. Filters need normalized-category storage and owned result copies. Ranking needs owned result copies, interest/event tokens, and O(log N) sorting stack space. `limit` reduces the returned result count, but all candidates are still scored and sorted.

See `docs/raymond-report-sections.md` for report source and integration caveats.

## Remaining team deliverables

Hayden owns the Trie, autocomplete, Trie tests, time/memory benchmark harness, charts, and video. Phuc owns dataset generation, the committed 1,000-event sample, the locally generated full dataset, query intersections, and the console menu. The group still needs to verify interfaces, integrate those components, collect benchmark evidence, assemble the final five-page 12-point report, record the video, and submit all feedback and deliverables.

Do not commit binaries, build directories, or the full dataset. `/data/events_full.csv` is ignored; coordinate any alternative full-data output path before generating it. Commit the generator and the agreed 1,000-event sample. Each member should review and understand their contributions and verify their own reflection before submission.

The original course template and GitHub workflow instructions are preserved in [resources/course-workflow.md](resources/course-workflow.md).
