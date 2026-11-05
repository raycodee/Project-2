#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gevents {

using EventId = std::uint32_t;
using PostingList = std::vector<EventId>;

struct Event {
    EventId eventId = 0;
    std::string title;
    std::string description;
    std::string category;
    std::string date;
    std::string time;
    std::string location;
    std::string organizer;
    std::vector<std::string> tags;
};

}
