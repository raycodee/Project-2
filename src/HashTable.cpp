#include "gevents/HashTable.h"
#include "gevents/Text.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace gevents {

HashTable::HashTable(std::size_t initialBuckets)
    : buckets_(std::max<std::size_t>(initialBuckets, 1)) {}

HashTable::~HashTable() {
    clear();
}

std::size_t HashTable::hash(std::string_view keyword) noexcept {
    std::uint64_t value = 14695981039346656037ULL;
    for (unsigned char c : keyword) {
        value ^= c;
        value *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(value);
}

bool HashTable::insert(std::string_view keyword, EventId eventId) {
    const auto key = normalizeKeyword(keyword);
    if (key.empty()) {
        return false;
    }
    auto index = hash(key) % buckets_.size();
    for (auto* node = buckets_[index].get(); node; node = node->next.get()) {
        if (node->keyword == key) {
            auto position = std::lower_bound(node->postings.begin(), node->postings.end(), eventId);
            if (position != node->postings.end() && *position == eventId) {
                return false;
            }
            node->postings.insert(position, eventId);
            return true;
        }
    }
    auto node = std::make_unique<Node>();
    node->keyword = key;
    node->postings.push_back(eventId);
    if (static_cast<double>(size_ + 1) / static_cast<double>(buckets_.size()) > 0.75) {
        if (buckets_.size() > std::numeric_limits<std::size_t>::max() / 2) {
            throw std::length_error("Hash table is too large to resize");
        }
        rehash(buckets_.size() * 2);
        index = hash(key) % buckets_.size();
    }
    node->next = std::move(buckets_[index]);
    buckets_[index] = std::move(node);
    ++size_;
    return true;
}

const PostingList* HashTable::find(std::string_view keyword) const {
    const auto key = normalizeKeyword(keyword);
    if (key.empty()) {
        return nullptr;
    }
    for (auto* node = buckets_[hash(key) % buckets_.size()].get(); node; node = node->next.get()) {
        if (node->keyword == key) {
            return &node->postings;
        }
    }
    return nullptr;
}

void HashTable::indexEvent(const Event& event) {
    const auto addText = [this, &event](std::string_view text) {
        for (const auto& token : tokenize(text)) {
            insert(token, event.eventId);
        }
    };
    addText(event.title);
    addText(event.description);
    addText(event.category);
    for (const auto& tag : event.tags) {
        addText(tag);
    }
}

void HashTable::rehash(std::size_t bucketCount) {
    std::vector<std::unique_ptr<Node>> resized(bucketCount);
    for (auto& head : buckets_) {
        while (head) {
            auto node = std::move(head);
            head = std::move(node->next);
            const auto index = hash(node->keyword) % bucketCount;
            node->next = std::move(resized[index]);
            resized[index] = std::move(node);
        }
    }
    buckets_.swap(resized);
}

void HashTable::clear() noexcept {
    for (auto& head : buckets_) {
        while (head) {
            auto node = std::move(head);
            head = std::move(node->next);
        }
    }
    size_ = 0;
}

std::size_t HashTable::size() const noexcept {
    return size_;
}

std::size_t HashTable::bucketCount() const noexcept {
    return buckets_.size();
}

double HashTable::loadFactor() const noexcept {
    return static_cast<double>(size_) / static_cast<double>(buckets_.size());
}

std::size_t HashTable::bucketIndex(std::string_view keyword) const {
    return hash(normalizeKeyword(keyword)) % buckets_.size();
}

}
