#include <thread>
#include <atomic>
#include "atomic.hpp"

template <class T>
class Node {
public:
    T* value;
    atomic_stamped<Node<T>> nextref;

    Node(T* v) : value(v), nextref(nullptr, 0) {}
};

template <class T>
class ConcurrentReuseQueue {
private:
    atomic_stamped<Node<T>> headref = atomic_stamped<Node<T>>(nullptr, 0);
    atomic_stamped<Node<T>> tailref = atomic_stamped<Node<T>>(nullptr, 0);
    thread_local static Node<T>* freelist;

public:
    ConcurrentReuseQueue() {
        Node<T>* node = new Node<T>(nullptr);
        headref.set(node, 0);
        tailref.set(node, 0);
    }

    Node<T>* allocate(T* value) {
        uint64_t stamp;
        Node<T>* node = freelist;
        if (node == nullptr)
            node = new Node<T>(value);
        else
            freelist = node->nextref.get(stamp);
        node->value = value;
        return node;
    }

    void free(Node<T>* node) {
        Node<T>* free = freelist;
        node->nextref.set(free, 0);
        freelist = node;
    }

    void enqueue(T* value) {
        Node<T>* node = new Node<T>(value);
        uint64_t tailStamp;
        uint64_t nextStamp;
        uint64_t stamp;

        while (true) {
            Node<T>* tail = tailref.get(tailStamp);
            Node<T>* next = tail->nextref.get(nextStamp);
            if (tail == tailref.get(stamp) && stamp == tailStamp) {
                if (next == nullptr) {
                    if (tail->nextref.cas(next, node, nextStamp, nextStamp+1)) {
                        tailref.cas(tail, node, tailStamp, tailStamp+1);
                        return;
                    }
                } else {
                    tailref.cas(tail, next, tailStamp, tailStamp+1);
                }
            }
        }
    }

    T* dequeue() {
        uint64_t tailStamp;
        uint64_t headStamp;
        uint64_t nextStamp;
        uint64_t stamp;

        while (true) {
            Node<T>* head = headref.get(headStamp);
            Node<T>* tail = tailref.get(tailStamp);
            Node<T>* next = head->nextref.get(nextStamp);
            if (head == headref.get(stamp) && stamp == headStamp) {
                if (head == tail) {
                    if (next == nullptr)
                        return nullptr;
                    tailref.cas(tail, next, tailStamp, tailStamp+1);
                } else {
                    T* value = next->value;
                    if (headref.cas(head, next, headStamp, headStamp+1)) {
                        free(head);
                        return value;
                    }
                }
            }
        }
    }
};

template <typename T>
thread_local Node<T>* ConcurrentReuseQueue<T>::freelist = nullptr;
