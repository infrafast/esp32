#ifndef _VALUES_h
#define _VALUES_h

#include <stdint.h>
#include <new>      // pour std::nothrow

/*
values handled by mixers are shared between strips and interfaces.
They are pointers which allow immediate propagation to the GUI and other strips
when one value is modified.

Each stored object contains:
- a float value       : usually a level
- a bool toggle       : usually mute / on-off
- one or two indexes  : typically strip/channel and optional mix/bus
- a count             : number of users reserving the same object
*/


/*
Exemple d’utilisation pour deux index

TwoIndexValueList sends;

TwoIndexValue* s1 = sends.reserve(5, 2);   // channel 5 -> mix 2
TwoIndexValue* s2 = sends.reserve(5, 2);   // même objet partagé

s1->toggle = true;

sends.unreserve(s1);
sends.unreserve(s2);                       // suppression ici
*/


struct OneIndexValue {
    float value;        // usually level
    bool toggle;        // usually on/off or mute

    uint8_t i1;         // index 1 = usually channel #
    uint8_t count;      // reservation count

    OneIndexValue()
        : value(0.0f), toggle(false), i1(0), count(0) {}
};

struct TwoIndexValue : public OneIndexValue {
    uint8_t i2;         // index 2 = usually bus/mix #

    TwoIndexValue()
        : OneIndexValue(), i2(0) {}
};

/*
===============================================================================
Generic linked list storing individually allocated nodes
===============================================================================

Goal:
- factorize all common logic between OneIndexValueList and TwoIndexValueList
- keep stable pointers to stored objects
- reuse the same reserve/get/unreserve/clear/size algorithms

How it works:
- each item is stored in its own dynamically allocated node
- therefore pointer returned to data remains stable
  as long as the node is not deleted
- reserve(...) searches an existing object with a user-provided matcher
  if found:
      increments count
      returns stable pointer
  else:
      allocates a new node
      initializes the object through a user-provided initializer
      sets count = 1
      inserts node at head
      returns pointer to data
- unreserve(...) decrements count and deletes node if count becomes 0
- get(...) searches using a matcher and returns the data pointer or nullptr
*/

template<typename T>
class ValueList {
private:
    struct Node {
        T data;
        Node* next;

        Node() : data(), next(nullptr) {}
    };

    Node* head;

public:
    ValueList() : head(nullptr) {}

    ~ValueList() {
        clear();
    }

    // Avoid accidental copy of the linked list
    ValueList(const ValueList&) = delete;
    ValueList& operator=(const ValueList&) = delete;

    /*
    reserve(match, init)

    Parameters:
    - match: function/lambda taking const T& and returning true if this is the object we want
    - init : function/lambda used to initialize a newly created object

    Behavior:
    - if object already exists: increment count and return pointer
    - otherwise create a new node, initialize it, set count=1, return pointer
    */
    template<typename Matcher, typename Initializer>
    T* reserve(Matcher match, Initializer init) {
        Node* current = head;

        // Search for an existing object
        while (current != nullptr) {
            if (match(current->data)) {
                current->data.count++;
                return &current->data;
            }
            current = current->next;
        }

        // Not found -> create new node
        Node* newNode = new (std::nothrow) Node();
        if (newNode == nullptr) {
            return nullptr;
        }

        // Initialize data using caller-provided logic
        init(newNode->data);

        // First reservation
        newNode->data.count = 1;

        // Insert at list head
        newNode->next = head;
        head = newNode;

        return &newNode->data;
    }

    /*
    unreserve(ptr)

    Decrements count of the object identified by ptr.
    If count reaches 0, removes the node from the list and deletes it.

    Returns:
    - true  if ptr was found in the list
    - false if ptr is nullptr or not found
    */
    bool unreserve(T* ptr) {
        if (ptr == nullptr) {
            return false;
        }

        Node* current = head;
        Node* previous = nullptr;

        while (current != nullptr) {
            if (&current->data == ptr) {
                // Decrement only if count is not already zero
                if (current->data.count > 0) {
                    current->data.count--;
                }

                // Delete node if nobody uses it anymore
                if (current->data.count == 0) {
                    if (previous == nullptr) {
                        head = current->next;
                    } else {
                        previous->next = current->next;
                    }
                    delete current;
                }

                return true;
            }

            previous = current;
            current = current->next;
        }

        return false;
    }

    /*
    get(match)

    Searches an object using a provided matcher.
    Returns:
    - pointer to object if found
    - nullptr otherwise
    */
    template<typename Matcher>
    T* get(Matcher match) const {
        Node* current = head;

        while (current != nullptr) {
            if (match(current->data)) {
                return &current->data;
            }
            current = current->next;
        }

        return nullptr;
    }

    /*
    clear()

    Deletes all nodes in the list.
    All returned pointers become invalid after this call.
    */
    void clear() {
        Node* current = head;

        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }

        head = nullptr;
    }

    /*
    size()

    Returns the number of currently stored distinct objects.
    This is NOT the sum of reservation counts.
    */
    uint16_t size() const {
        uint16_t n = 0;
        Node* current = head;

        while (current != nullptr) {
            n++;
            current = current->next;
        }

        return n;
    }
};

/*
===============================================================================
Wrapper for values indexed by one index
===============================================================================

Public API kept close to the original version:
- reserve(i1)
- unreserve(ptr)
- get(i1)
- clear()
- size()
*/

class OneIndexValueList {
private:
    ValueList<OneIndexValue> list;

public:
    OneIndexValueList() = default;
    ~OneIndexValueList() = default;

    OneIndexValueList(const OneIndexValueList&) = delete;
    OneIndexValueList& operator=(const OneIndexValueList&) = delete;

    /*
    reserve(i1)

    If an object with index i1 already exists:
    - increments count
    - returns pointer to existing object

    Otherwise:
    - creates a new object
    - initializes i1
    - count = 1
    - returns pointer to it
    */
    OneIndexValue* reserve(uint8_t i1) {
        return list.reserve(
            // matcher
            [i1](const OneIndexValue& v) {
                return (v.i1 == i1);
            },
            // initializer for a new object
            [i1](OneIndexValue& v) {
                v = OneIndexValue();
                v.i1 = i1;
            }
        );
    }

    /*
    unreserve(ptr)

    Decrements the reservation count.
    Deletes the object if count becomes 0.
    */
    bool unreserve(OneIndexValue* ptr) {
        return list.unreserve(ptr);
    }

    /*
    get(i1)

    Returns the pointer to the object matching i1, or nullptr.
    */
    OneIndexValue* get(uint8_t i1) const {
        return list.get(
            [i1](const OneIndexValue& v) {
                return (v.i1 == i1);
            }
        );
    }

    void clear() {
        list.clear();
    }

    uint16_t size() const {
        return list.size();
    }
};

/*
===============================================================================
Wrapper for values indexed by two indexes
===============================================================================

Public API kept close to the original version:
- reserve(i1, i2)
- unreserve(ptr)
- get(i1, i2)
- clear()
- size()
*/

class TwoIndexValueList {
private:
    ValueList<TwoIndexValue> list;

public:
    TwoIndexValueList() = default;
    ~TwoIndexValueList() = default;

    TwoIndexValueList(const TwoIndexValueList&) = delete;
    TwoIndexValueList& operator=(const TwoIndexValueList&) = delete;

    /*
    reserve(i1, i2)

    If an object with indexes (i1, i2) already exists:
    - increments count
    - returns pointer to existing object

    Otherwise:
    - creates a new object
    - initializes i1 and i2
    - count = 1
    - returns pointer to it
    */
    TwoIndexValue* reserve(uint8_t i1, uint8_t i2) {
        return list.reserve(
            // matcher
            [i1, i2](const TwoIndexValue& v) {
                return (v.i1 == i1) && (v.i2 == i2);
            },
            // initializer for a new object
            [i1, i2](TwoIndexValue& v) {
                v = TwoIndexValue();
                v.i1 = i1;
                v.i2 = i2;
            }
        );
    }

    /*
    unreserve(ptr)

    Decrements the reservation count.
    Deletes the object if count becomes 0.
    */
    bool unreserve(TwoIndexValue* ptr) {
        return list.unreserve(ptr);
    }

    /*
    get(i1, i2)

    Returns the pointer to the object matching (i1, i2), or nullptr.
    */
    TwoIndexValue* get(uint8_t i1, uint8_t i2) const {
        return list.get(
            [i1, i2](const TwoIndexValue& v) {
                return (v.i1 == i1) && (v.i2 == i2);
            }
        );
    }

    void clear() {
        list.clear();
    }

    uint16_t size() const {
        return list.size();
    }
};


#endif



