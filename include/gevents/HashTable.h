#pragma once

#include "gevents/Event.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gevents {

class HashTable {
public:
    explicit HashTable(std::size_t initialBuckets = 16);
    ~HashTable();
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;
    HashTable(HashTable&&) = delete;
    HashTable& operator=(HashTable&&) = delete;

    bool insert(std::string_view keyword, EventId eventId);
    const PostingList* find(std::string_view keyword) const;
    void indexEvent(const Event& event);
    void clear() noexcept;
    std::size_t size() const noexcept;
    std::size_t bucketCount() const noexcept;
    double loadFactor() const noexcept;
    std::size_t bucketIndex(std::string_view keyword) const;

private:
    struct Node {
        std::string keyword;
        PostingList postings;
        std::unique_ptr<Node> next;
    };

    std::vector<std::unique_ptr<Node>> buckets_;
    std::size_t size_ = 0;

    static std::size_t hash(std::string_view keyword) noexcept;
    void rehash(std::size_t bucketCount);
};

}
