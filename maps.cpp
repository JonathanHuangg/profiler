// rule of 5: destructor, copy, copy assignment, move, move assignment
#include <iostream>
#include <functional>
#include <string>
#include <utility>
template <typename K, typename V>

// linear chaining
class UnorderedMapChain_norm {
    private:
        struct HashNode {
            K key;
            V value;
            HashNode* next; 

            HashNode(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
        };

        HashNode** buckets; 
        size_t capacity;
        size_t size; 
        float max_load_factor;

        size_t get_bucket_idx(const K& k) {
            std::hash<K> hashFunc;
            return hashFunc(k) % capacity;
        }
    public:
        UnorderedMapChain_norm(size_t cap = 16) : capacity(cap), size(0), max_load_factor(0.75f) {
            buckets = new HashNode*[capacity]();
        }

        // destructor
        ~UnorderedMapChain_norm() {
            clear();
            delete[] buckets;
        }

        void clear() {
            for (size_t i = 0; i < capacity; ++i) {
                HashNode* curr = buckets[i];
                while (curr != nullptr) {
                    HashNode* next_node = curr->next;
                    delete curr; 
                    curr = next_node;
                }
                buckets[i] = nullptr;
            }
            size = 0;
        }

        // copy
        UnorderedMapChain_norm(const UnorderedMapChain_norm& other) : capacity(other.capacity), size(other.size), max_load_factor(other.max_load_factor) {
            buckets = new HashNode*[capacity]();

            for (size_t i = 0; i < capacity; ++i) {
                if (other.buckets[i] != nullptr) {
                    HashNode* curr_other = other.buckets[i];
                    buckets[i] = new HashNode(curr_other->key, curr_other->value);
                    HashNode* curr_this = buckets[i];

                    while (curr_other->next) {
                        curr_other = curr_other->next;
                        curr_this->next = new HashNode(curr_other->key, curr_other->value);
                        curr_this = curr_this->next;
                    }
                }
            }
        }

        // copy and move assignment. use swap because two hashmaps will 
        // seldom have the same structure (simiarily, do swaps with trees)
        UnorderedMapChain_norm& operator=(UnorderedMapChain_norm other) noexcept {
            swap(*this, other);
            return *this; 
        }

        // move
        UnorderedMapChain_norm(UnorderedMapChain_norm&& other) noexcept 
            : buckets(other.buckets), capacity(other.capacity), size(other.size), max_load_factor(other.max_load_factor) {
                other.buckets = nullptr;
                other.capacity = 0;
                other.size = 0;
        }

        void clear() {
            if (!buckets) {
                return; 
            }

            for (size_t i = 0; i < capacity; ++i) {
                HashNode* current = buckets[i]
                while (current != nullptr) {
                    HashNode* next_node = current->next; 
                    delete current;
                    current = next_node; 
                }
                buckets[i] = nullptr;
            }
            size = 0;
        }
};