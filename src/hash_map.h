// hash_map.h — a hand-written hash map with chained buckets.
//
// This is the completed version of maps.cpp, which did not compile: it
// defined clear() twice (once missing a semicolon), had no insert/find/
// erase at all, and its copy/move story was half finished. The fixes:
//   * one clear(), null-safe so a moved-from map can be destroyed;
//   * insert / find / erase / contains, with rehashing at the load factor;
//   * copy-and-swap assignment via a friend swap, so copy and move
//     assignment share one correct path (two maps rarely share a shape,
//     so swapping beats merging buckets).
//
// Rule of 5 is honoured throughout: destructor, copy constructor,
// copy assignment, move constructor, move assignment.
//
// Usage:
//   UnorderedMapChain_norm<int, std::string> map;
//   map.insert(42, "answer");
//   if (auto* v = map.find(42)) { ... }
#pragma once

#include <cstddef>
#include <functional>
#include <utility>

template <typename K, typename V>
class UnorderedMapChain_norm {
  private:
    struct HashNode {
        K key;
        V value;
        HashNode* next;

        HashNode(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    HashNode** buckets;
    std::size_t capacity;
    std::size_t size_;
    float max_load_factor;

    std::size_t bucket_index(const K& k) const {
        return std::hash<K>{}(k) % capacity;
    }

    void rehash(std::size_t new_capacity) {
        HashNode** new_buckets = new HashNode*[new_capacity]();
        for (std::size_t i = 0; i < capacity; ++i) {
            HashNode* curr = buckets[i];
            while (curr != nullptr) {
                HashNode* next = curr->next;
                const std::size_t idx = std::hash<K>{}(curr->key) % new_capacity;
                curr->next = new_buckets[idx];
                new_buckets[idx] = curr;
                curr = next;
            }
        }
        delete[] buckets;
        buckets = new_buckets;
        capacity = new_capacity;
    }

  public:
    explicit UnorderedMapChain_norm(std::size_t cap = 16)
        : buckets(new HashNode*[cap]()), capacity(cap), size_(0), max_load_factor(0.75f) {}

    ~UnorderedMapChain_norm() {
        clear();
        delete[] buckets;
    }

    // Copy: deep-copy every chain, bucket by bucket.
    UnorderedMapChain_norm(const UnorderedMapChain_norm& other)
        : buckets(new HashNode*[other.capacity]()),
          capacity(other.capacity),
          size_(other.size_),
          max_load_factor(other.max_load_factor) {
        for (std::size_t i = 0; i < capacity; ++i) {
            if (other.buckets[i] == nullptr) continue;
            const HashNode* curr_other = other.buckets[i];
            buckets[i] = new HashNode(curr_other->key, curr_other->value);
            HashNode* curr_this = buckets[i];
            while (curr_other->next != nullptr) {
                curr_other = curr_other->next;
                curr_this->next = new HashNode(curr_other->key, curr_other->value);
                curr_this = curr_this->next;
            }
        }
    }

    // Move: steal the buckets; leave the source empty but destructible.
    UnorderedMapChain_norm(UnorderedMapChain_norm&& other) noexcept
        : buckets(other.buckets),
          capacity(other.capacity),
          size_(other.size_),
          max_load_factor(other.max_load_factor) {
        other.buckets = nullptr;
        other.capacity = 0;
        other.size_ = 0;
    }

    // Copy-and-swap assignment: one path for copy and move assignment.
    UnorderedMapChain_norm& operator=(UnorderedMapChain_norm other) noexcept {
        swap(*this, other);
        return *this;
    }

    friend void swap(UnorderedMapChain_norm& a, UnorderedMapChain_norm& b) noexcept {
        using std::swap;
        swap(a.buckets, b.buckets);
        swap(a.capacity, b.capacity);
        swap(a.size_, b.size_);
        swap(a.max_load_factor, b.max_load_factor);
    }

    void clear() {
        if (buckets == nullptr) return;
        for (std::size_t i = 0; i < capacity; ++i) {
            HashNode* curr = buckets[i];
            while (curr != nullptr) {
                HashNode* next_node = curr->next;
                delete curr;
                curr = next_node;
            }
            buckets[i] = nullptr;
        }
        size_ = 0;
    }

    // Insert or overwrite. Returns true if the key was new.
    bool insert(const K& key, const V& value) {
        if (buckets == nullptr) return false;
        if (static_cast<float>(size_ + 1) > max_load_factor * static_cast<float>(capacity)) {
            rehash(capacity * 2);
        }
        const std::size_t idx = bucket_index(key);
        for (HashNode* curr = buckets[idx]; curr != nullptr; curr = curr->next) {
            if (curr->key == key) {
                curr->value = value;
                return false;
            }
        }
        HashNode* node = new HashNode(key, value);
        node->next = buckets[idx];
        buckets[idx] = node;
        ++size_;
        return true;
    }

    // Returns a pointer to the value, or nullptr if the key is absent.
    // The pointer is invalidated by the next insert that rehashes.
    V* find(const K& key) {
        if (buckets == nullptr) return nullptr;
        for (HashNode* curr = buckets[bucket_index(key)]; curr != nullptr; curr = curr->next) {
            if (curr->key == key) return &curr->value;
        }
        return nullptr;
    }

    const V* find(const K& key) const {
        if (buckets == nullptr) return nullptr;
        for (const HashNode* curr = buckets[bucket_index(key)]; curr != nullptr; curr = curr->next) {
            if (curr->key == key) return &curr->value;
        }
        return nullptr;
    }

    bool contains(const K& key) const { return find(key) != nullptr; }

    bool erase(const K& key) {
        if (buckets == nullptr) return false;
        const std::size_t idx = bucket_index(key);
        HashNode* curr = buckets[idx];
        HashNode* prev = nullptr;
        while (curr != nullptr) {
            if (curr->key == key) {
                if (prev == nullptr) {
                    buckets[idx] = curr->next;
                } else {
                    prev->next = curr->next;
                }
                delete curr;
                --size_;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    std::size_t bucket_count() const { return capacity; }
    float load_factor() const {
        return capacity == 0 ? 0.0f : static_cast<float>(size_) / static_cast<float>(capacity);
    }
};
