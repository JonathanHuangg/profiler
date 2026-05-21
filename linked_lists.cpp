/*
No heap allocation becasue of memory fragmentation and cache misses

lvalue is  on the heap. they have an address so you use &
lvalue needs to be deep copied
Rvalue is on the stack or in registers so no memory address. 
rvalue is pointer stealing (move)
&& is a rvalue reference

rule of 5: destructor, copy constructor, copy assignment operator, move constructor, move assignment operator
*/

template <typename T> 
class SinglyLinkedList_bare {
    private:

        struct Node {
            T data; 
            Node* next; 
            
            // using template U makes compiler to a type deduction at runtime to see lvalue or rvalue
            template <typename U>
            // passing l values does a deep copy, passing r value is a move operation
            // Node(T val, Node* nxt = nullptr) : data(std::move(val)), next(nxt) {}
            // Node(const T& val, Node* nxt = nullptr) : data(val), next(nxt) {}
            Node(U&& val, Node* nxt=nullptr)
                : data(std::forward<U>(val)), next(nxt) {}
        };

        Node* head;
        std::size_t list_size;
    
    public:
        SinglyLinkedList_bare() : head(nullptr), list_size(0) {}

        ~SinglyLinkedList_bare() { clear(); }
        
        // get rid of original copy and copy operator
        SinglyLinkedList_bare(const SinglyLinkedList_bare&) = delete; 
        SinglyLinkedList_bare& operator = (const SinglyLinkedList_bare&) = delete;

        // enable shallow copy. noexcept guarantees it never throws exception
        SinglyLinkedList_bare(SinglyLinkedList_bare && other) noexcept 
            : head(other.head), list_size(other.list_size) {
                other.head = nullptr;
                other.list_size = 0;
        }

        SinglyLinkedList_bare& operator = (SinglyLinkedList_bare&& other) noexcept {
            if (this != &other) {
                clear();
                head = other.head;
                list_size = other.list_size;
                other.head = nullptr;
                other.list_size = 0;
            }
            return *this;
        }

        void clear() {
            Node* probe = head;
            while (probe != nullptr) {
                Node* next = probe->next;
                delete probe;
                probe = next;
            }

            head = nullptr;
            list_size = 0;
        }

        // emplace takes raw arguments and builds direcly inside constructor
        // push takes an already created object
        template <typename... Args>
        void emplace_front(Args&&... args) {
            Node* new_node = new Node(std::forward<Args>(args)...);
            new_node->next = head;
            head = new_node;
            ++list_size;
        }   
        /*
        std::string test = "hello";
        list.push_front(test);
        
        1) because it is a string, lvalue -> template U is std::string&
        2) push_front(std::string&) -> emplace_front(test)
        3) in emplace, args is an lvalue reference
        4) Node constructor runs using std::string copy constructor

        list.push_front(std::string("hello world"));
        1) this is now a rvalue so U is std::string
        2) push_front(std::string&& val)
        3) in emplace, this rvalues must be pointer stolen/moved -> emplace_front(std::move(val))
        4) Args sees this as std::string
        5) new Node() is called and because it is rvalue, std::forward casts ravlue std::string into rvalue reference &&std::string&&
        6) Node constructor runs
        */
        void push_front(U&& val) {
            emplace_front(std::forward<U>(val));
        }

        void pop_front() {
            if (head == nullptr) {
                return;
            }

            Node* old_head = head;
            head = head.next;
            delete old_head;
            --list_size--;
        }

        std::size_t size() const {return list_size;}
        bool empty() const {return list_size == 0;}
};

// destructor
// copy constructor
// copy assignment
// move constructor
// move assignment
template <typename T> 
class SinglyLinkedList_norm {
    private:
        struct Node {
            T data;
            Node* next;

            // no direct initialization
            Node(T val, Node*nxt=nullptr) {
                data = val;
                next = nxt; 
            }
        };
        
        Node* head; 
        std::size_t list_size;
    
    public:
        // constructor
        SinglyLinkedList_norm() : head(nullptr), list_size(0) {}

        ~SinglyLinkedList_norm() {clear();}

        // copy constructor. called when new object is being created
        // SinglyLinkedList_norm ListA;
        // SinglyLinkedList_norm ListB(ListA)
        // SinglyLinkedList_norm ListC = ListA;
        SinglyLinkedList_norm(const SinglyLinkedList_norm& other) : head(nullptr), list_size(0) {
            if (other.head == nullptr) {
                return;
            }

            head = new Node(other.head->data);
            Node* this_probe = head;
            Node* other_probe = other.head->next;

            while (other_probe != nullptr) {
                this_probe->next = new Node(other_probe->data);
                other_probe = other_probe->next;
                this_probe = this_probe->next;
            }
            list_size = other.list_size;
        }

        // copy assigment operator -> overwriting
        // SinglyLinkedList_norm ListA;
        // SinglyLinkedList_norm ListB;
        // ListB = ListA;
        SinglyLinkedList_norm& operator= (const SinglyLinkedList_norm& other) {
            if (this != &other) {
                clear();

                if (other.head == nullptr) {
                    return;
                }
                head = new Node(other.head->data);
                Node* this_probe = head; 
                Node* other_probe = other.head->next;

                while (other_probe != nullptr) {
                    this_probe->next = new Node(other_probe->data);
                    other_probe = other_probe->next;
                    this_probe = this_probe->next; 
                }

                list_size = other.list_size;
            }

            return *this;
        }

        // move constructor -> steals the pointers
        SinglyLinkedList_norm(SinglyLinkedList_norm&& other) noexcept : head(other.head), list_size(other.list_size) {
            other.head = nullptr;
            other.list_size = 0;
        }       

        void clear() {
            Node* probe = head;

            while (probe != nullptr) {
                Node* next_node = probe->next;
                delete probe;
                probe = next_node;
            }
            head = nullptr;
            list_size = 0;
        }

        void push_front(T val) {
            head = new Node(val, head);
            ++list_size;
        }
};

class DoublyLinkedList {
    NULL; 
}

class CircularlyLinkedList {
    NULL; 
}