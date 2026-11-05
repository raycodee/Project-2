#include "gevents/HashTable.h"
#include "gevents/Text.h"
#include "TestSupport.h"

#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using gevents::HashTable;
using gevents::PostingList;

void testEmptyAndNormalization() {
    HashTable table(0);
    require(table.size() == 0 && table.bucketCount() == 1, "Zero bucket request must be usable");
    require(table.find("missing") == nullptr, "Missing key should return null");
    require(!table.insert("!", 3) && table.find("") == nullptr, "Empty tokens must be ignored");
    require(table.insert(" ROBOTICS! ", 7), "First insert failed");
    require(!table.insert("robotics", 7), "Duplicate posting must be ignored");
    require(*table.find("Robotics") == PostingList{7}, "Case normalization failed");
    requireInvalid([&] { table.insert("two words", 1); }, "Multiword insertion must be rejected");
    requireInvalid([&] { table.find("two words"); }, "Multiword lookup must be rejected");
    require(gevents::tokenize("UF's C++ Club 2026") ==
            std::vector<std::string>{"uf", "s", "c", "club", "2026"}, "Tokenization contract changed");
}

void testCollisionChain() {
    HashTable table(64);
    std::vector<std::string> keys;
    for (int i = 0; keys.size() < 4; ++i) {
        auto key = "collision" + std::to_string(i);
        if (table.bucketIndex(key) == 0) {
            keys.push_back(key);
        }
    }
    for (std::size_t i = 0; i < keys.size(); ++i) {
        table.insert(keys[i], static_cast<gevents::EventId>(i));
    }
    require(table.bucketCount() == 64, "Collision test unexpectedly resized");
    for (std::size_t i = 0; i < keys.size(); ++i) {
        require(*table.find(keys[i]) == PostingList{static_cast<gevents::EventId>(i)},
                "A collision overwrote a neighboring entry");
    }
}

void testPostingOrderAndRehash() {
    HashTable table(2);
    for (auto id : {9U, 0U, 4U, 9U, 2U, 4U}) {
        table.insert("shared", id);
    }
    require(*table.find("shared") == PostingList({0, 2, 4, 9}), "Postings must be sorted and unique");
    const auto* retained = table.find("shared");
    for (unsigned int i = 0; i < 5000; ++i) {
        table.insert("term" + std::to_string(i), i);
        require(table.loadFactor() <= 0.75, "Resize threshold exceeded");
    }
    require(table.size() == 5001, "size must count distinct keywords");
    require(retained == table.find("shared") && *retained == PostingList({0, 2, 4, 9}),
            "Rehash invalidated the posting-list object");
    for (unsigned int i = 0; i < 5000; ++i) {
        require(*table.find("term" + std::to_string(i)) == PostingList{i}, "Rehash lost an entry");
    }
    const auto buckets = table.bucketCount();
    table.clear();
    require(table.size() == 0 && table.find("shared") == nullptr, "Clear left stale data");
    require(table.bucketCount() == buckets, "Clear should retain allocated buckets");
    table.insert("reused", 42);
    require(*table.find("reused") == PostingList{42}, "Reuse after clear failed");
}

void testRandomizedReference() {
    HashTable table;
    std::vector<PostingList> reference(200);
    std::mt19937 random(75);
    for (int i = 0; i < 20000; ++i) {
        const auto key = random() % reference.size();
        const auto id = static_cast<gevents::EventId>(random() % 1000);
        table.insert("key" + std::to_string(key), id);
        auto& expected = reference[key];
        if (std::find(expected.begin(), expected.end(), id) == expected.end()) {
            expected.push_back(id);
        }
    }
    for (std::size_t i = 0; i < reference.size(); ++i) {
        std::sort(reference[i].begin(), reference[i].end());
        require(*table.find("key" + std::to_string(i)) == reference[i], "Randomized reference mismatch");
    }
}

void testEventIndexing() {
    HashTable table;
    gevents::Event event;
    event.eventId = 12;
    event.title = "Robotics Robotics!";
    event.description = "Build a robot";
    event.category = "Technology";
    event.tags = {"C++", "robotics"};
    table.indexEvent(event);
    table.indexEvent(event);
    for (const auto& token : {"robotics", "build", "a", "robot", "technology", "c"}) {
        require(*table.find(token) == PostingList{12}, "Event field missing or duplicated");
    }
    require(table.size() == 6, "Repeated indexing must not add keywords");
}

void testLargeInput() {
    HashTable table;
    for (unsigned int i = 0; i < 100000; ++i) {
        table.insert("event" + std::to_string(i), i);
        table.insert("campus", i);
    }
    require(table.size() == 100001, "Large input keyword count mismatch");
    require(table.find("campus")->size() == 100000, "Large posting list lost IDs");
    for (unsigned int i = 0; i < 100000; ++i) {
        require(*table.find("event" + std::to_string(i)) == PostingList{i}, "Large input lookup mismatch");
        require((*table.find("campus"))[i] == i, "Large posting list order mismatch");
    }
}

int main() {
    try {
        testEmptyAndNormalization();
        testCollisionChain();
        testPostingOrderAndRehash();
        testRandomizedReference();
        testEventIndexing();
        testLargeInput();
        std::cout << "Hash table: 6 test groups passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
