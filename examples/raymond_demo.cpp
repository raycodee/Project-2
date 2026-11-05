#include "gevents/Filters.h"
#include "gevents/HashTable.h"
#include "gevents/Ranking.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <vector>

int main() {
    try {
        const std::vector<gevents::Event> events = {
            {101, "Coding Workshop", "Build robotics projects", "Technology", "2026-11-05",
             "18:00", "Demo Room A", "Example Club", {"coding", "robotics"}},
            {102, "Music Social", "Meet other students", "Social", "2026-11-06",
             "17:00", "Demo Lawn", "Example Club", {"music"}},
            {103, "Startup Night", "Discuss new ideas", "Entrepreneurship", "2026-11-07",
             "19:00", "Demo Room B", "Example Club", {"startups"}},
            {104, "Coding Lab", "Practice coding", "Technology", "2026-11-12",
             "16:00", "Demo Room A", "Example Club", {"coding"}},
            {105, "Coding Social", "Find study partners", "Social", "2026-11-08",
             "15:00", "Demo Lawn", "Example Club", {"coding"}},
            {106, "Coding Lecture", "Explore coding", "Technology", "2026-11-09",
             "14:00", "Demo Room B", "Example Club", {"learning"}}
        };
        gevents::HashTable index;
        for (const auto& event : events) {
            index.indexEvent(event);
        }
        std::cout << "Synthetic integration example\nKeyword: coding\nPosting IDs:";
        const auto* postings = index.find("CODING");
        std::vector<gevents::Event> candidates;
        if (postings) {
            for (const auto id : *postings) {
                std::cout << ' ' << id;
            }
            for (const auto& event : events) {
                if (std::binary_search(postings->begin(), postings->end(), event.eventId)) {
                    candidates.push_back(event);
                }
            }
        }
        const auto filter = gevents::weekStartingOn("2026-11-05", {"technology"});
        const auto filtered = gevents::filterEvents(candidates, filter);
        const auto ranked = gevents::rankEvents(filtered, {"coding", "robotics"});
        std::cout << "\nCategory: Technology\nDates: " << *filter.startDate << " through "
                  << *filter.endDate << "\nInterests: coding, robotics\n";
        for (const auto& result : ranked) {
            const auto& event = result.event;
            std::cout << event.eventId << " | " << event.title << " | " << event.date
                      << ' ' << event.time << " | " << event.location
                      << " | score=" << result.relevanceScore << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
