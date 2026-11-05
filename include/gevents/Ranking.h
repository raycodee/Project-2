#pragma once

#include "gevents/Event.h"

#include <cstddef>
#include <string>
#include <vector>

namespace gevents {

struct RankedEvent {
    Event event;
    std::size_t relevanceScore = 0;
};

std::size_t scoreEvent(const Event& event, const std::vector<std::string>& interests);
std::vector<RankedEvent> rankEvents(const std::vector<Event>& events,
                                   const std::vector<std::string>& interests,
                                   std::size_t limit = 0);

}
