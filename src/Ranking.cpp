#include "gevents/Ranking.h"
#include "gevents/Filters.h"
#include "gevents/Text.h"

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

namespace gevents {
namespace {

using Tokens = std::vector<std::string>;

Tokens uniqueTokens(Tokens tokens) {
    std::sort(tokens.begin(), tokens.end());
    tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());
    return tokens;
}

Tokens listTokens(const std::vector<std::string>& texts) {
    Tokens tokens;
    for (const auto& text : texts) {
        auto next = tokenize(text);
        tokens.insert(tokens.end(), next.begin(), next.end());
    }
    return uniqueTokens(std::move(tokens));
}

std::size_t fieldScore(const Tokens& field, const Tokens& interests, std::size_t weight) {
    std::size_t score = 0;
    for (const auto& interest : interests) {
        if (std::binary_search(field.begin(), field.end(), interest)) {
            score += weight;
        }
    }
    return score;
}

std::size_t preparedScore(const Event& event, const Tokens& interests) {
    if (interests.empty()) {
        return 0;
    }
    return fieldScore(listTokens(event.tags), interests, 5) +
           fieldScore(uniqueTokens(tokenize(event.category)), interests, 3) +
           fieldScore(uniqueTokens(tokenize(event.title)), interests, 2) +
           fieldScore(uniqueTokens(tokenize(event.description)), interests, 1);
}

bool validTime(std::string_view time) {
    if (time.empty()) {
        return true;
    }
    return time.size() == 5 && time[2] == ':' && time[0] >= '0' && time[0] <= '2' &&
           time[1] >= '0' && time[1] <= '9' && time[3] >= '0' && time[3] <= '5' &&
           time[4] >= '0' && time[4] <= '9' && (time[0] != '2' || time[1] <= '3');
}

}

std::size_t scoreEvent(const Event& event, const std::vector<std::string>& interests) {
    return preparedScore(event, listTokens(interests));
}

std::vector<RankedEvent> rankEvents(const std::vector<Event>& events,
                                   const std::vector<std::string>& interests,
                                   std::size_t limit) {
    const auto tokens = listTokens(interests);
    std::vector<RankedEvent> ranked;
    ranked.reserve(events.size());
    for (const auto& event : events) {
        if (!isValidDate(event.date) || !validTime(event.time)) {
            throw std::invalid_argument("Ranking requires YYYY-MM-DD dates and empty or HH:MM times");
        }
        ranked.push_back({event, preparedScore(event, tokens)});
    }
    std::sort(ranked.begin(), ranked.end(), [](const RankedEvent& left, const RankedEvent& right) {
        if (left.relevanceScore != right.relevanceScore) {
            return left.relevanceScore > right.relevanceScore;
        }
        return std::tie(left.event.date, left.event.time, left.event.eventId) <
               std::tie(right.event.date, right.event.time, right.event.eventId);
    });
    if (limit != 0 && ranked.size() > limit) {
        ranked.resize(limit);
    }
    return ranked;
}

}
