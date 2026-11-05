#include "gevents/Ranking.h"
#include "gevents/Filters.h"
#include "gevents/HashTable.h"
#include "TestSupport.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

gevents::Event makeEvent(gevents::EventId id, const std::string& title,
                        const std::string& date = "2026-11-05", const std::string& time = "18:00") {
    gevents::Event event;
    event.eventId = id;
    event.title = title;
    event.date = date;
    event.time = time;
    return event;
}

void testScores() {
    auto event = makeEvent(1, "Coding coding");
    event.category = "Coding";
    event.description = "Learn CODING! coding";
    event.tags = {"coding", "Coding"};
    require(gevents::scoreEvent(event, {"coding"}) == 11, "Field weights must sum to 11");
    require(gevents::scoreEvent(event, {"CODING! coding", "coding"}) == 11,
            "Repeated interests or event words inflated score");
    require(gevents::scoreEvent(event, {"code"}) == 0, "Matching must use full tokens");
    require(gevents::scoreEvent(event, {"!!!"}) == 0, "Punctuation-only interests must score zero");
    require(gevents::scoreEvent(event, {}) == 0, "No interests must score zero");
    event.tags.push_back("robotics");
    require(gevents::scoreEvent(event, {"coding robotics"}) == 16, "Multiword interest tokenization failed");
    auto tagOnly = makeEvent(2, "Other");
    tagOnly.tags = {"coding"};
    require(gevents::scoreEvent(tagOnly, {"coding"}) == 5, "Tag weight changed");
    auto categoryOnly = makeEvent(3, "Other");
    categoryOnly.category = "coding";
    require(gevents::scoreEvent(categoryOnly, {"coding"}) == 3, "Category weight changed");
    require(gevents::scoreEvent(makeEvent(4, "Coding"), {"coding"}) == 2, "Title weight changed");
    auto descriptionOnly = makeEvent(5, "Other");
    descriptionOnly.description = "coding";
    require(gevents::scoreEvent(descriptionOnly, {"coding"}) == 1, "Description weight changed");
}

void testOrderingAndLimits() {
    std::vector<gevents::Event> events = {
        makeEvent(4, "Coding", "2026-11-06"), makeEvent(3, "Coding", "2026-11-05", "19:00"),
        makeEvent(2, "Coding"), makeEvent(1, "Coding"), makeEvent(5, "Other", "2026-11-04")};
    const auto results = gevents::rankEvents(events, {"coding"});
    const std::vector<gevents::EventId> expected{1, 2, 3, 4, 5};
    for (std::size_t i = 0; i < results.size(); ++i) {
        require(results[i].event.eventId == expected[i], "Score or tie-break ordering failed");
    }
    require(gevents::rankEvents(events, {"coding"}, 2).size() == 2, "Top-k truncation failed");
    require(gevents::rankEvents(events, {"coding"}, 100).size() == 5, "Large limit failed");
    require(gevents::rankEvents({}, {"coding"}).empty(), "Empty ranking failed");
    require(gevents::rankEvents(events, {}).front().event.eventId == 5, "Zero scores must use chronological ties");
    std::reverse(events.begin(), events.end());
    const auto reversed = gevents::rankEvents(events, {"coding"});
    for (std::size_t i = 0; i < reversed.size(); ++i) {
        require(reversed[i].event.eventId == expected[i], "Ranking depends on input order");
    }
    events[0].title = "Changed";
    require(results.back().event.title == "Other", "Ranked results must own their event copies");
}

void testValidation() {
    requireInvalid([] { gevents::rankEvents({makeEvent(1, "x", "bad")}, {}); }, "Invalid date accepted");
    for (const auto& time : {"24:00", "09:60", "9:00", "12:xx", "-1:00"}) {
        requireInvalid([&] { gevents::rankEvents({makeEvent(1, "x", "2026-11-05", time)}, {}); },
                       "Invalid time accepted");
    }
    require(gevents::rankEvents({makeEvent(1, "x", "2026-11-05", "")}, {}).size() == 1,
            "Empty time should be allowed");
    require(gevents::rankEvents({makeEvent(1, "x", "2026-11-05", "23:59")}, {}).size() == 1,
            "Valid time rejected");
}

void testIndexFilterRankIntegration() {
    auto coding = makeEvent(1, "Coding workshop");
    coding.category = "Technology";
    coding.tags = {"coding"};
    auto social = makeEvent(2, "Coding social");
    social.category = "Social";
    auto later = makeEvent(3, "Coding lecture", "2026-11-12");
    later.category = "Technology";
    const std::vector<gevents::Event> events{coding, social, later};
    gevents::HashTable index;
    for (const auto& event : events) {
        index.indexEvent(event);
    }
    const auto* postings = index.find("CODING");
    require(postings && *postings == gevents::PostingList({1, 2, 3}), "Index integration mismatch");
    std::vector<gevents::Event> candidates;
    for (const auto& event : events) {
        if (std::binary_search(postings->begin(), postings->end(), event.eventId)) {
            candidates.push_back(event);
        }
    }
    const auto filtered = gevents::filterEvents(candidates,
        gevents::weekStartingOn("2026-11-05", {"technology"}));
    const auto ranked = gevents::rankEvents(filtered, {"coding"});
    require(ranked.size() == 1 && ranked[0].event.eventId == 1 && ranked[0].relevanceScore == 7,
            "Index/filter/rank pipeline failed");
}

int main() {
    try {
        testScores();
        testOrderingAndLimits();
        testValidation();
        testIndexFilterRankIntegration();
        std::cout << "Ranking: 4 test groups passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
