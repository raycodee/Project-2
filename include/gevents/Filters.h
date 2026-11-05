#pragma once

#include "gevents/Event.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gevents {

struct EventFilter {
    std::vector<std::string> categories;
    std::optional<std::string> startDate;
    std::optional<std::string> endDate;
};

bool isValidDate(std::string_view date) noexcept;
EventFilter weekStartingOn(const std::string& startDate,
                           std::vector<std::string> categories = {});
std::vector<Event> filterEvents(const std::vector<Event>& events, const EventFilter& filter);

}
