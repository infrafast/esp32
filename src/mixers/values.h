#ifndef _VALUES_h
#define _VALUES_h

#include <stdint.h>
#include "Interval.h"  // Generic stable-address reservation container

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
Example with two indices

TwoIndexValueList sends;

TwoIndexValue* s1 = sends.reserve(5, 2);   // channel 5 -> mix 2
TwoIndexValue* s2 = sends.reserve(5, 2);   // same shared object

s1->toggle = true;

sends.unreserve(s1);
sends.unreserve(s2);                       // deleted here
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
    TwoIndexValue* get(uint8_t i1, uint8_t i2) {
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



