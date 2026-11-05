#include "gevents/Filters.h"
#include "TestSupport.h"

#include <iostream>
#include <string>
#include <vector>

gevents::Event makeEvent(gevents::EventId id, const std::string& date,
                        const std::string& category = "Technology") {
    gevents::Event event;
    event.eventId = id;
    event.date = date;
    event.category = category;
    return event;
}

void testCalendarValidation() {
    for (const auto& date : {"2024-02-29", "2000-02-29", "0001-01-01", "9999-12-31", "2026-11-06"}) {
        require(gevents::isValidDate(date), "Valid calendar date rejected");
    }
    for (const auto& date : {"", "2026-2-03", "2026-02-29", "1900-02-29", "2026-04-31",
                             "0000-01-01", "2026-00-10", "2026-13-01", "2026-01-00",
                             "2026/01/01", "202a-01-01", "2026-01-01 extra"}) {
        require(!gevents::isValidDate(date), "Invalid calendar date accepted");
    }
}

void testBoundsAndCategories() {
    const std::vector<gevents::Event> events = {
        makeEvent(1, "2026-11-04"), makeEvent(2, "2026-11-05", " SOCIAL "),
        makeEvent(3, "2026-11-06"), makeEvent(4, "2026-11-07", "Technology"),
        makeEvent(5, "2026-11-06", "Data Science")};
    const auto filtered = gevents::filterEvents(events, {{"technology"}, "2026-11-05", "2026-11-06"});
    require(filtered.size() == 1 && filtered[0].eventId == 3, "Combined filters failed");
    const auto bounds = gevents::filterEvents(events, {{}, "2026-11-05", "2026-11-06"});
    require(bounds.size() == 3 && bounds[0].eventId == 2 && bounds[2].eventId == 5,
            "Date endpoints must be inclusive and input order preserved");
    require(gevents::filterEvents(events, {{}, "2026-11-06", std::nullopt}).size() == 3,
            "Open end range failed");
    require(gevents::filterEvents(events, {{}, std::nullopt, "2026-11-05"}).size() == 2,
            "Open start range failed");
    require(gevents::filterEvents(events, {{"social", "DATA-science"}, {}, {}}).size() == 2,
            "Category OR matching or normalization failed");
    require(gevents::filterEvents(events, {}).size() == events.size(), "No filters should retain all events");
    require(gevents::filterEvents(events, {{"unknown"}, {}, {}}).empty(), "Unknown category should match nothing");
    require(gevents::filterEvents({}, {}).empty(), "Empty dataset failed");
    require(events[0].eventId == 1 && events[1].category == " SOCIAL ", "Filtering mutated input");
}

void testWeeks() {
    require(*gevents::weekStartingOn("2026-11-05").endDate == "2026-11-11", "Week must cover seven dates");
    require(*gevents::weekStartingOn("2024-02-26").endDate == "2024-03-03", "Leap week failed");
    require(*gevents::weekStartingOn("2026-12-28").endDate == "2027-01-03", "Year boundary failed");
    require(*gevents::weekStartingOn("9999-12-25").endDate == "9999-12-31", "Last valid week failed");
    requireInvalid([] { gevents::weekStartingOn("9999-12-26"); }, "Overflowing week accepted");
    const std::vector<gevents::Event> events = {
        makeEvent(1, "2026-11-04"), makeEvent(2, "2026-11-05"),
        makeEvent(3, "2026-11-11"), makeEvent(4, "2026-11-12")};
    const auto results = gevents::filterEvents(events, gevents::weekStartingOn("2026-11-05"));
    require(results.size() == 2 && results.front().eventId == 2 && results.back().eventId == 3,
            "Week endpoint filtering failed");
}

void testInvalidInputs() {
    requireInvalid([] { gevents::filterEvents({}, {{}, "2026-11-06", "2026-11-05"}); },
                   "Reversed range accepted");
    requireInvalid([] { gevents::filterEvents({}, {{}, "2026-02-30", {}}); }, "Invalid bound accepted");
    requireInvalid([] { gevents::filterEvents({}, {{"!!!"}, {}, {}}); }, "Empty category accepted");
    requireInvalid([] { gevents::weekStartingOn("2026-02-29"); }, "Invalid week start accepted");
    requireInvalid([] { gevents::filterEvents({makeEvent(1, "bad")}, {}); }, "Invalid event date accepted");
}

int main() {
    try {
        testCalendarValidation();
        testBoundsAndCategories();
        testWeeks();
        testInvalidInputs();
        std::cout << "Filters: 4 test groups passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
