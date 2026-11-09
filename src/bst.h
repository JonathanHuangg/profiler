// bst.h — a binary search tree with a cache-line node-layout experiment.
//
// This is the completed version of tree.cpp, which did not compile:
// missing semicolons/braces (struct definitions, a constructor, insert),
// and the feature-test guard for std::hardware_destructive_interference_size
// was inverted — it used the constant when the macro was *not* defined and
// fell back to 64 when it was.
//
// The experiment the file was circling: does padding each node out to a
// full cache line help or hurt? Two layouts with the same interface:
//   * BSTAligned — every node occupies its own cache line (alignas on the
//     struct, so sizeof(Node) is a multiple of CACHE_LINE). A node never
//     shares a line with its neighbour: no false sharing, but more memory.
//   * BSTPacked — nodes allocated back-to-back, the way `new` lays them
//     out. The baseline the aligned layout is measured against.
// The benchmark in main.cpp runs both and prints the difference, which is
// the point of the file.
//
// Copy is deleted on purpose: two trees sharing nodes would double-free
// when both go out of scope. Move is allowed.
#pragma once

#include <cstddef>
#include <new>

// Use the standard constant when the library provides it; otherwise 64,
// the cache-line size on every x86-64 / arm64 machine this targets.
#ifdef __cpp_lib_hardware_interference_size
constexpr std::size_t CACHE_LINE = std::hardware_destructive_interference_size;
#else
constexpr std::size_t CACHE_LINE = 64;
#endif

// Aligned-node BST: every node occupies its own cache line.
template <typename T>
class BSTAligned {
  private:
    struct alignas(CACHE_LINE) Node {
        T data;
        Node* left;
        Node* right;
        Node(T val) : data(val), left(nullptr), right(nullptr) {}
    };

    Node* root;
    std::size_t size_;

    void clear(Node* node) {
        if (node == nullptr) return;
        clear(node->left);
        clear(node->right);
        delete node;
    }

  public:
    BSTAligned() : root(nullptr), size_(0) {}
    ~BSTAligned() { clear(root); }

    BSTAligned(const BSTAligned&) = delete;
    BSTAligned& operator=(const BSTAligned&) = delete;
    BSTAligned(BSTAligned&& other) noexcept : root(other.root), size_(other.size_) {
        other.root = nullptr;
        other.size_ = 0;
    }
    BSTAligned& operator=(BSTAligned&& other) noexcept {
        if (this != &other) {
            clear(root);
            root = other.root;
            size_ = other.size_;
            other.root = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void insert(T val) {
        Node** probe = &root;
        while (*probe != nullptr) {
            probe = (val < (*probe)->data) ? &((*probe)->left) : &((*probe)->right);
        }
        *probe = new Node(val);
        ++size_;
    }

    bool contains(T key) const {
        Node* probe = root;
        while (probe != nullptr) {
            if (probe->data == key) return true;
            probe = (key < probe->data) ? probe->left : probe->right;
        }
        return false;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
};

// Packed-node BST: nodes allocated back-to-back, the `new` default.
template <typename T>
class BSTPacked {
  private:
    struct Node {
        T data;
        Node* left;
        Node* right;
        Node(T val) : data(val), left(nullptr), right(nullptr) {}
    };

    Node* root;
    std::size_t size_;

    void clear(Node* node) {
        if (node == nullptr) return;
        clear(node->left);
        clear(node->right);
        delete node;
    }

  public:
    BSTPacked() : root(nullptr), size_(0) {}
    ~BSTPacked() { clear(root); }

    BSTPacked(const BSTPacked&) = delete;
    BSTPacked& operator=(const BSTPacked&) = delete;
    BSTPacked(BSTPacked&& other) noexcept : root(other.root), size_(other.size_) {
        other.root = nullptr;
        other.size_ = 0;
    }
    BSTPacked& operator=(BSTPacked&& other) noexcept {
        if (this != &other) {
            clear(root);
            root = other.root;
            size_ = other.size_;
            other.root = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void insert(T val) {
        Node** probe = &root;
        while (*probe != nullptr) {
            probe = (val < (*probe)->data) ? &((*probe)->left) : &((*probe)->right);
        }
        *probe = new Node(val);
        ++size_;
    }

    bool contains(T key) const {
        Node* probe = root;
        while (probe != nullptr) {
            if (probe->data == key) return true;
            probe = (key < probe->data) ? probe->left : probe->right;
        }
        return false;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
};
