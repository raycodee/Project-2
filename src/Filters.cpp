#include "gevents/Filters.h"
#include "gevents/Text.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace gevents {
namespace {

struct DateParts {
    int year;
    int month;
    int day;
};

int daysInMonth(int year, int month) noexcept {
    constexpr int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    return days[month - 1] + (month == 2 && leap ? 1 : 0);
}

DateParts dateParts(std::string_view date) noexcept {
    return {(date[0] - '0') * 1000 + (date[1] - '0') * 100 +
                (date[2] - '0') * 10 + date[3] - '0',
            (date[5] - '0') * 10 + date[6] - '0',
            (date[8] - '0') * 10 + date[9] - '0'};
}

void validateDate(std::string_view date) {
    if (!isValidDate(date)) {
        throw std::invalid_argument("Expected a real date in YYYY-MM-DD format: " + std::string(date));
    }
}

}

bool isValidDate(std::string_view date) noexcept {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }
    for (std::size_t i = 0; i < date.size(); ++i) {
        if (i != 4 && i != 7 && (date[i] < '0' || date[i] > '9')) {
            return false;
        }
    }
    const auto parts = dateParts(date);
    return parts.year >= 1 && parts.month >= 1 && parts.month <= 12 &&
           parts.day >= 1 && parts.day <= daysInMonth(parts.year, parts.month);
}

EventFilter weekStartingOn(const std::string& startDate, std::vector<std::string> categories) {
    validateDate(startDate);
    auto parts = dateParts(startDate);
    for (int i = 0; i < 6; ++i) {
        ++parts.day;
        if (parts.day > daysInMonth(parts.year, parts.month)) {
            parts.day = 1;
            ++parts.month;
            if (parts.month > 12) {
                parts.month = 1;
                ++parts.year;
            }
        }
        if (parts.year > 9999) {
            throw std::invalid_argument("Seven-day window exceeds year 9999");
        }
    }
    std::ostringstream end;
    end << std::setfill('0') << std::setw(4) << parts.year << '-'
        << std::setw(2) << parts.month << '-' << std::setw(2) << parts.day;
    return {std::move(categories), startDate, end.str()};
}

std::vector<Event> filterEvents(const std::vector<Event>& events, const EventFilter& filter) {
    if (filter.startDate) {
        validateDate(*filter.startDate);
    }
    if (filter.endDate) {
        validateDate(*filter.endDate);
    }
    if (filter.startDate && filter.endDate && *filter.startDate > *filter.endDate) {
        throw std::invalid_argument("Start date must not be after end date");
    }
    std::vector<std::string> categories;
    categories.reserve(filter.categories.size());
    for (const auto& category : filter.categories) {
        auto normalized = normalizeCategory(category);
        if (normalized.empty()) {
            throw std::invalid_argument("Category filter must contain letters or digits");
        }
        categories.push_back(std::move(normalized));
    }
    std::vector<Event> matches;
    for (const auto& event : events) {
        validateDate(event.date);
        if ((filter.startDate && event.date < *filter.startDate) ||
            (filter.endDate && event.date > *filter.endDate)) {
            continue;
        }
        if (!categories.empty()) {
            const auto category = normalizeCategory(event.category);
            if (std::find(categories.begin(), categories.end(), category) == categories.end()) {
                continue;
            }
        }
        matches.push_back(event);
    }
    return matches;
}

}
