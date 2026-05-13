#include <iostream> 
#include <new> 
#include <cstddef> 

template <typename T>
/*
BST:
left < right
*/
class BST {
    private:
        struct Node {
            T data; // T sized data
            Node* left; // 8 byte pointers
            Node* right; // 8 byte pointers
            bool block_friendly; // 1 byte

            // constructor is empty because everything is done in initializer
            // putting it inside brackets is assignment and inefficeint because data memory is first created and then copied in
            Node(T val, bool friendly) : data(val), left(nullptr), right(nullptr), block_friendly(friendly) {}
        };

        Node* root;
        bool pad;
        void clear(Node* node) {
            if (!node) {
                return;
            }

            clear(node->left);
            clear(node->right);

            delete node;
        }
    public:
        // initialize it as null_ptr so there's not a need for dummy node
        BST(bool friendly) : root(nullptr), block_friendly(friendly){}
        ~BST() {
            clear(root);
        }

        Node* search(Node* root, T key) {
            Node* probe = root;
            while (probe != nullptr) {
                if (probe->data == key) {
                    return probe; 
                }

                probe = (key < probe->data) ? probe->left : probe->right;
            }
            return nullptr; 
        }

        void insert(T val) {
            Node** probe = &root;
            while (*probe) {
                if (val < ((*probe)->data)) {
                    probe = &((*probe->left));
                } else {
                    probe = &((*probe->right));
                }
            }
            *probe = new Node(val)
        } 

        /*
        if you do BST a = b, both point to the same memory
        when a leaves scope and freed, b later will try to also free causing double free

        just don't use these constructors
        */
        // this is the copy constructor
        BST(const BST&) = delete;
        // copy assignment operator: what happens when you assign one BST to another
        BST& operator = (const BST&) = delete; 

        /*
        bc i don't seem to remember:
        
        Node* ptr -> ptr is a variable, it points to the memory address. ex, ptr can be 0x100 at 0x100, a Node exists
        &ptr -> address of the ptr itself
        *ptr -> go to the value of ptr (0x100) and get the node address 

        let object Node be at 0x100
        Node* ptr; // value of ptr is 0x100. ptr is at 0x500
        Node** addr; // this is the address if ptr and identical to &ptr. The value of addr is 0x500. 
        */
}



/*
Red Black Tree:
1) all nodes red/black
2) root and leaves are black
3) if node is red, children are black
4) all paths from node -> NIL descendents is same number of black nodes
5) longest path is no more than 2x length of shortest (all black vs red/black interleaving)

*/