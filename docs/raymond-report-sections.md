# Raymond Shi — technical report source

This is source material for Group 75's combined report, not a submission-ready report. Condense it alongside Hayden's and Phuc's sections to fit the maximum five pages at 12-point font, including references. Verify the final team scope, proposal-change rationale, benchmark findings, usernames, repository link, and video link before export.

## Implemented features and responsibilities

Raymond's assigned components are the handwritten hash index, hash-table tests, date/category filters, relevance scoring/ranking, and run instructions. The code provides reusable C++17 modules and a six-record synthetic integration example. It does not yet include Hayden's Trie or benchmark harness, or Phuc's generated dataset, multi-keyword query intersections, parser, and interactive menu.

The hash index maps a normalized keyword to a sorted, duplicate-free list of event IDs. It implements deterministic 64-bit FNV-1a hashing, bucket selection, separate linked collision chains, insertion, lookup, and resizing explicitly. The index does not use an associative standard-library container. Vectors store bucket heads and posting IDs; uniquely owned nodes represent chains. Before inserting a new keyword would push the load factor above 0.75, the table doubles its bucket count and relinks existing nodes. Rehashing preserves posting lists without copying their ID vectors. Chain cleanup is iterative to avoid a recursion depth proportional to a long collision chain.

Shared normalization lowercases ASCII letters and splits text at bytes other than ASCII letters and digits. Indexing covers titles, descriptions, categories, and tags. Repeated words and repeated indexing do not duplicate an event ID. Single-keyword lookup returns an immutable view of the posting-list vector; multi-keyword interpretation is left to the query layer. Unique event IDs identify records independently of vector position.

Category filters combine selected categories with OR, then combine that result with date constraints using AND. Matching ignores ASCII case and normalizes punctuation and whitespace. Dates are validated Gregorian dates in fixed YYYY-MM-DD format. Bounds are inclusive and may be open on either side. A week helper constructs exactly seven dates beginning with a supplied date, including month/year changes and leap days. Invalid inputs throw explicit exceptions. Filtering preserves input order and returns owned event copies.

Relevance scoring tokenizes and deduplicates interests. Each distinct interest earns five points for a tag match, three for category, two for title, and one for description. Repetition within one field does not increase that field's contribution. Results sort by descending score, followed by ascending date, time, and ID. These weights are a transparent baseline for the group to review. The implementation uses the standard sorting algorithm for this supporting feature. A positive limit truncates after the full sort; zero returns every candidate, including zero-score events.

## Changes from the original proposal

The original proposal compared Merge Sort with Quick Sort. The revised team working plan assigns Hayden a Trie and Raymond a hash table. Both structures can index the same normalized keywords and return event postings, supporting a comparison of exact-keyword access; Trie prefix autocomplete is an additional feature rather than an equivalent hash lookup. The final report should state the team's actual reason for this change after the team confirms it. Do not describe Merge Sort or Quick Sort as implemented in this revision.

A shared event record, common tokenizer, deterministic tie rules, explicit invalid-input handling, and a small example were added to make module integration reviewable. The example is a correctness demonstration, not the full application or benchmark dataset.

## Worst-case analysis

Let U denote distinct keywords, B buckets, P total keyword/ID postings, p postings for one keyword, and w maximum keyword length. A lookup can traverse all U nodes if every keyword collides, with O(w(U + 1)) worst-case time. With well-distributed hashes, expected lookup time is O(w(1 + U/B)). Resizing reduces average load but does not eliminate worst-case collisions.

Insertion without resizing costs O(w(U + 1) + p): it scans a collision chain and may shift p IDs in a sorted vector. Binary search alone is O(log p), so it would be incorrect to call the entire posting insertion logarithmic. A resize costs O(B + Uw) for new bucket initialization, hashing, and relinking nodes. Thus insertion including resizing is O(w(U + 1) + p + B). Index storage is O(B + Uw + P); resizing additionally requires O(B) bucket storage. Appending monotonically increasing IDs avoids middle-of-vector shifts but still requires duplicate checking.

For filtering, let N be candidate events, C selected categories, A total selected-category text length, W maximum category length, L maximum event text length, and S copied result payload bytes. The bound is O(A + N(L + CW) + S), accounting for normalization and linear category membership checks. Without category selections, fixed-format date validation and comparisons give O(N + S). Output copies and normalized selections require additional storage.

For ranking, let I be total interest text length, K interest tokens before deduplication, and T maximum tokens in an event's scored fields. Tokenization, deduplication, and binary-search scoring cost O(I + wK log(K + 1) + N(L + wT log(T + 1) + wK log(T + 1))). Sorting adds O(N log(N + 1)) comparisons because the date/time/ID tie fields have bounded size. Copying the candidate records adds O(S). Ranking stores owned results and token lists, with logarithmic sorting stack space. Returning only a few results still scores and sorts all candidates.

## Validation and limitations

On this Mac with Apple Clang 17, the Release build passed all three CTest executables, comprising fourteen test groups. Hash-table checks cover empty and normalized input, deliberately colliding keys, sorted/unique postings, repeated resizing, randomized comparison with an independent vector reference, record indexing, and 100,000 generated IDs. Filter checks cover inclusive/open bounds, category matching, real calendar dates, leap years, invalid ranges, and seven-day windows. Ranking checks cover each field weight, repeated interests, exact-token matching, ties, limits, malformed dates/times, and an index/filter/rank integration pipeline.

The six-record example returned Coding Workshop with score 13 and Coding Lecture with score 3 after the selected keyword, Technology category, and seven-day date window were applied. A separate direct-compiler build reproduced the result. A UBSan-only Debug build passed all three executables. AddressSanitizer executables hung inside runtime/dynamic-loader initialization before the tests began, including a bounded retry outside the sandbox; address-safety coverage remains unverified on this machine.

The 100,000-ID test is an index stress check, not the team's full event dataset. No comparative time/memory findings or speedup claims have been collected here. The tokenizer is ASCII-only; the index does not support deletion, mutation, or concurrent access. Final performance comparisons must use equivalent normalized records and workloads, and should report dataset sizes, compiler settings, repetitions, timing boundaries, and the definition of memory measurement.

## Reflection input Raymond should verify

Technical observations supported by this implementation include the difference between average hash access and worst-case collision behavior, the linear shifting cost hidden inside sorted posting insertion, the importance of shared normalization, and the distinction between a test failure and a toolchain startup failure.

Raymond should write his personal reflection in his own words after reviewing the code: which parts he understands and learned, what challenged him, how collaboration actually went, and what he would change next time. The group should supply its own experience and workflow reflection. Those experiences are not established by the code or test results.

## References for the combined report

The source artifacts for these sections are the repository implementation, tests, README, the group's revised working plan, and the supplied COP3530 Project 2b requirements. The University of Florida events calendar was identified in the proposal as domain context; the local example records are synthetic and were not downloaded from that calendar. Add any algorithm references actually used by the group to the final report's reference list.
