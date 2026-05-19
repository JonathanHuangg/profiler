/*
No heap allocation becasue of memory fragmentation and cache misses
*/

template <typename T> 
class SinglyLinkedList_bare {
    private:

        struct Node {
            T data; 
            Node* next; 

            template <typename U>
            Node(U&& val, Node* nxt=nullptr)
                : data(std::forward<U>(val)), next(nxt) {}
        }
}

class DoublyLinkedList {
    NULL; 
}

class CircularlyLinkedList {
    NULL; 
}